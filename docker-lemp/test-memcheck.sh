#!/bin/bash

#### tpcc_bench variables
TPCC_PATH=/home/mkwon/bench-lemp/tpcc-mysql
DB_NAME=tpcc_bench

#### Parameters
NUM_DEV=4
NUM_PHP=5
NUM_SCALE=1
ARR_CONNECT=(5)
DOCKER_ROOT=/var/lib/docker

prepare_firewall() {
	echo "$(tput setaf 4 bold)$(tput setab 7)Prepare firewall$(tput sgr 0)"
	## This allows that we can 'service iptables save' in CentOS7
	sudo systemctl disable firewalld
	sudo yum install -y iptables-services net-tools
	sudo systemctl enable iptables
}

pid_waits() {
    echo "$(tput setaf 4 bold)$(tput setab 7)Waiting the pids$(tput sgr 0)"
    PIDS=("${!1}")
    for pid in "${PIDS[*]}"; do
        wait $pid
    done
}

pid_kills() {
    echo "$(tput setaf 4 bold)$(tput setab 7)Kill the pids$(tput sgr 0)"
	PIDS=("${!1}")
	for pid in "${PIDS[*]}"; do
		kill -15 $pid
	done
}

nvme_format() {
    echo "$(tput setaf 4 bold)$(tput setab 7)Format nvme block devices$(tput sgr 0)"
    for DEV_ID in $(seq 1 ${NUM_DEV}); do
        nvme format /dev/nvme2n${DEV_ID} -n ${DEV_ID} --ses=0
    done
    sleep 1

    FLAG=true
    while $FLAG; do
        NUSE="$(nvme id-ns /dev/nvme2n1 -n 1 | grep nuse | awk '{print $3}')"
        if [[ $NUSE -eq "0" ]]; then
            FLAG=false
            echo "nvme format done"
        fi
    done
    sleep 1
}

nvme_flush() {
    echo "$(tput setaf 4 bold)$(tput setab 7)Flush nvme block devices$(tput sgr 0)"
    for DEV_ID in $(seq 1 ${NUM_DEV}); do
        nvme flush /dev/nvme2n${DEV_ID}
    done
}

docker_remove() {
    echo "$(tput setaf 4 bold)$(tput setab 7)Start removing existing docker$(tput sgr 0)"
    docker ps -aq | xargs --no-run-if-empty docker stop \
    && docker ps -aq | xargs --no-run-if-empty docker rm \
	&& docker system prune --all -f \
    && systemctl stop docker

	rm -rf ${DOCKER_ROOT}

    for DEV_ID in $(seq 1 4); do
        if mountpoint -q /mnt/nvme2n${DEV_ID}; then
            umount /mnt/nvme2n${DEV_ID}
        fi
        rm -rf /mnt/nvme2n${DEV_ID} \
        && mkdir -p /mnt/nvme2n${DEV_ID} \
        && wipefs --all --force /dev/nvme2n${DEV_ID}
    done

	targets=($(brctl show | grep br- | awk '{print $1}'))
	for target in ${targets[@]}; do
		ifconfig $target down
		brctl delbr $target
	done
}

docker_init() {
    echo "$(tput setaf 4 bold)$(tput setab 7)Initializing docker engine$(tput sgr 0)"
    for DEV_ID in $(seq 1 ${NUM_DEV}); do
        mkfs.xfs /dev/nvme2n${DEV_ID} \
        && mount /dev/nvme2n${DEV_ID} /mnt/nvme2n${DEV_ID}
    done

	count=1
	for DEV_ID in $(seq 1 4); do
		cp docker/database/my.cnf /mnt/nvme2n${DEV_ID}/my.cnf
		MNT_DIR=/mnt/nvme2n${DEV_ID}

		for SCALE_ID in $(seq 1 ${NUM_SCALE}); do
			NGINX_DIR=${MNT_DIR}/nginx${SCALE_ID}
			PHP_DIR=${MNT_DIR}/nginx${SCALE_ID}-php

			mkdir -p ${NGINX_DIR} ${PHP_DIR}
			mkdir -p ${NGINX_DIR}/nginx-log ${NGINX_DIR}/webpage
			for PHP_ID in $(seq 1 ${NUM_PHP}); do
				mkdir -p ${PHP_DIR}/php-log${PHP_ID}
			done

			cp docker/webpage/index.php ${NGINX_DIR}/webpage/index.php
			cp docker/php/www.conf ${PHP_DIR}/php.conf
			cp docker/php/Dockerfile ${PHP_DIR}/Dockerfile
			cp docker/nginx/default.conf ${NGINX_DIR}/nginx.conf
			cp docker/nginx/Dockerfile ${NGINX_DIR}/Dockerfile

			sed -i "s|nginx:9000|nginx${count}:9000|" ${PHP_DIR}/php.conf
			sed -i "s|\${SCALE_ID}|${count}|" ${NGINX_DIR}/nginx.conf
			sed -i "s|HOST_NAME|database${DEV_ID}|" ${NGINX_DIR}/webpage/index.php
			sed -i "s|MYSQL_USER|username${SCALE_ID}|" ${NGINX_DIR}/webpage/index.php
			sed -i "s|MYSQL_PASSWORD|userpassword${SCALE_ID}|" ${NGINX_DIR}/webpage/index.php
			sed -i "s|MYSQL_DATABASE|tpcc_bench${SCALE_ID}|" ${NGINX_DIR}/webpage/index.php
			let count="$count + 1"
		done
	done

	iptables -t nat -N DOCKER
	iptables -t nat -A PREROUTING -m addrtype --dst-type LOCAL -j DOCKER
	iptables -t nat -A PREROUTING -m addrtype --dst-type LOCAL ! --dst 172.17.0.1/8 -j DOCKER

	service iptables save
	service iptables restart
	# iptables -L
	systemctl restart docker

	# >/var/log/messages
}

docker_healthy() {
    echo "$(tput setaf 4 bold)$(tput setab 7)Check docker healthy$(tput sgr 0)"
	while docker ps -a | grep -c 'starting\|unhealthy' > /dev/null;
	do
		sleep 1;
	done
}

mysql_db_gen() {
	echo "$(tput setaf 4 bold)$(tput setab 7)Generate Mysql DB data file$(tput sgr 0)"
	rm -rf ./docker/database/db-home/mysql-init-files/*
	for SCALE_ID in $(seq 1 ${NUM_SCALE}); do
		cp scripts/setup.sql setup${SCALE_ID}.sql
		sed -i "s|username|username${SCALE_ID}|" setup${SCALE_ID}.sql
		sed -i "s|userpassword|userpassword${SCALE_ID}|" setup${SCALE_ID}.sql
		sed -i "s|tpcc_bench|tpcc_bench${SCALE_ID}|" setup${SCALE_ID}.sql
		sed -i "s|\${MNT_DIR}|/mnt/nvme2n${DEV_ID}|" setup${SCALE_ID}.sql
		mv setup${SCALE_ID}.sql ./docker/database/db-home/mysql-init-files/
	done
}

dockerfile_gen() {
	## Cleanup 
	echo "$(tput setaf 4 bold)$(tput setab 7)Generating docker-compose file$(tput sgr 0)"
	rm -rf scripts/docker-compose.server.yml
	rm -rf scripts/docker-compose.db.yml
	
	cp scripts/docker-compose.db.yml.template scripts/docker-compose.db.yml

	count=1
	for DEV_ID in $(seq 1 4); do
		for SCALE_ID in $(seq 1 ${NUM_SCALE}); do
			NGINX_PORT=$((8079+${count}))
			cp scripts/docker-compose.server.yml.template scripts/docker-compose.NS${DEV_ID}-server${SCALE_ID}.yml
			sed -i "s|8080|${NGINX_PORT}|" scripts/docker-compose.NS${DEV_ID}-server${SCALE_ID}.yml
			sed -i "s|\${MNT_DIR}|/mnt/nvme2n${DEV_ID}|" scripts/docker-compose.NS${DEV_ID}-server${SCALE_ID}.yml
			sed -i "s|\${SCALE_ID}|${SCALE_ID}|" scripts/docker-compose.NS${DEV_ID}-server${SCALE_ID}.yml
			sed -i "s|\${DEV_ID}|${DEV_ID}|" scripts/docker-compose.NS${DEV_ID}-server${SCALE_ID}.yml
			sed -i "s|\${count}|${count}|" scripts/docker-compose.NS${DEV_ID}-server${SCALE_ID}.yml
			let count="$count + 1"
		done
	done

	## Merge multiple web service compose files
	for DEV_ID in $(seq 1 4); do
		for SCALE_ID in $(seq 1 ${NUM_SCALE}); do
			cat "scripts/docker-compose.NS${DEV_ID}-server${SCALE_ID}.yml" >> scripts/docker-compose.server.yml
		done
	done
	sed -i "1i\services:" scripts/docker-compose.server.yml
	sed -i "1i\version: '2.4'" scripts/docker-compose.server.yml
	echo "networks:" >> scripts/docker-compose.server.yml
	echo " shared1:" >> scripts/docker-compose.server.yml
	echo "  external:" >> scripts/docker-compose.server.yml
	echo "   name: shared1" >> scripts/docker-compose.server.yml
	echo " shared2:" >> scripts/docker-compose.server.yml
	echo "  external:" >> scripts/docker-compose.server.yml
	echo "   name: shared2" >> scripts/docker-compose.server.yml
	echo " shared3:" >> scripts/docker-compose.server.yml
	echo "  external:" >> scripts/docker-compose.server.yml
	echo "   name: shared3" >> scripts/docker-compose.server.yml
	echo " shared4:" >> scripts/docker-compose.server.yml
	echo "  external:" >> scripts/docker-compose.server.yml
	echo "   name: shared4" >> scripts/docker-compose.server.yml	
	## Cleanup temporal scripts
	rm -rf scripts/docker-compose.NS*
}

docker_web_gen() {
    echo "$(tput setaf 4 bold)$(tput setab 7)Generating web containers$(tput sgr 0)"
	
	for DEV_ID in $(seq 1 4); do
		if [[ -n $(docker network ls | grep shared${DEV_ID}) ]]; then
			echo "please cleanup docker network"
		else
			docker network create shared${DEV_ID}
		fi
	done

	### Build the docker-compose service containers
	docker-compose -f scripts/docker-compose.db.yml -f scripts/docker-compose.server.yml build --parallel database1 database2 database3 database4 
	for DEV_ID in $(seq 1 4); do
		for SCALE_ID in $(seq 1 ${NUM_SCALE}); do
			let CONT_ID="$SCALE_ID + $NUM_SCALE * ($DEV_ID - 1)"
			# ${NUM_PHP} 
			docker-compose -f scripts/docker-compose.db.yml -f scripts/docker-compose.server.yml build --parallel nginx${CONT_ID} nginx${CONT_ID}-php1 nginx${CONT_ID}-php2 nginx${CONT_ID}-php3 nginx${CONT_ID}-php4 nginx${CONT_ID}-php5
		done
	done

	### Up the docker-compose service containers
	docker-compose -f scripts/docker-compose.db.yml -f scripts/docker-compose.server.yml up -d database1 database2 database3 database4
	for DEV_ID in $(seq 1 4); do
		for SCALE_ID in $(seq 1 ${NUM_SCALE}); do
			let CONT_ID="$SCALE_ID + $NUM_SCALE * ($DEV_ID - 1)"
			# ${NUM_PHP} 
			docker-compose -f scripts/docker-compose.db.yml -f scripts/docker-compose.server.yml up -d nginx${CONT_ID}-php1 nginx${CONT_ID}-php2 nginx${CONT_ID}-php3 nginx${CONT_ID}-php4 nginx${CONT_ID}-php5
			docker_healthy
			docker-compose -f scripts/docker-compose.db.yml -f scripts/docker-compose.server.yml up -d nginx${CONT_ID}
		done
	done

	sleep 5
	docker_healthy
	
	for DEV_ID in $(seq 1 4); do
		for SCALE_ID in $(seq 1 ${NUM_SCALE}); do
			docker exec -i database${DEV_ID} mysql -uroot -proot <<< "source /home/mysql/setup"${SCALE_ID}".sql"
		done
	done
}

docker_web_init() {
	echo "$(tput setaf 4 bold)$(tput setab 7)Initializing web container tables$(tput sgr 0)"

	TPCC_PIDS=()
	for DEV_ID in $(seq 1 4); do
		CONT_IP="$(docker inspect -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' database${DEV_ID})" 
		for SCALE_ID in $(seq 1 ${NUM_SCALE}); do
			DB_NAME=tpcc_bench${SCALE_ID}
			USER_NAME=username${SCALE_ID}
			USER_PASSWD=userpassword${SCALE_ID}
			${TPCC_PATH}/tpcc_load -h $CONT_IP -d $DB_NAME -u ${USER_NAME} -p ${USER_PASSWD} -w 1 -s 1 & TPCC_PIDS+=("$!")
		done
	done
	pid_waits TPCC_PIDS[@]
	sleep 5
}

docker_web_run() {
	echo "$(tput setaf 4 bold)$(tput setab 7)Execute TPCC-bench$(tput sgr 0)"
	#### Change  service name to IP address to remove error
	#### 'php_network_getaddress: getaddrinfo failed: Temporary failure in name resolution'
	count=1
	for DEV_ID in $(seq 1 4); do
		MNT_DIR=/mnt/nvme2n${DEV_ID}
		for SCALE_ID in $(seq 1 ${NUM_SCALE}); do
			NGINX_DIR=${MNT_DIR}/nginx${SCALE_ID}
			IP_ADDR="$(docker inspect -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' database${DEV_ID})"
			sed -i "s|database${DEV_ID}|${IP_ADDR}|" ${NGINX_DIR}/webpage/index.php
			for PHP_ID in $(seq 1 ${NUM_PHP}); do
				SERVICE_NAME=nginx${count}-php${PHP_ID}
				IP_ADDR="$(docker inspect -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' ${SERVICE_NAME})"
				sed -i "s|${SERVICE_NAME}|${IP_ADDR}|" ${NGINX_DIR}/nginx.conf
			done
			let count="$count + 1"
		done
	done

	#### Execute tpcc_start
	count=1
	TPCC_PIDS=()
	for DEV_ID in $(seq 1 4); do
		for SCALE_ID in $(seq 1 ${NUM_SCALE}); do
			PORT_NUM=$((8079+${count}))
			${TPCC_PATH}/tpcc_start -h http://127.0.0.1:${PORT_NUM}/index.php -P ${PORT_NUM} -w 1 -c ${NUM_CONNECT} -r 10 -l 180 & TPCC_PIDS+=("$!")
			let count="$count + 1"
		done
	done
	pid_waits TPCC_PIDS[@] 
	sleep 5
}

run_monitor() {
	cd ../../../src/dockprom \
	&& ADMIN_USER=admin ADMIN_PASSWORD=admin docker-compose up -d \
	&& cd ../../bench-lemp/docker-lemp
}

step1() {
	docker_remove
	nvme_flush
	nvme_format

	docker_init
	mysql_db_gen
	dockerfile_gen
	run_monitor
	docker_web_gen
}

step2() {
	docker_web_init
	docker_web_run
}

for NUM_CONNECT in "${ARR_CONNECT[@]}"; do
	# step1

	### Plz open firefox (localhost:3000 for grafana) before start step2
	step2
done
