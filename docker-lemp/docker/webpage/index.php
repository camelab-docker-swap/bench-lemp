<?php
    // Disable error reporting on screen
    error_reporting(0);

    // Helper functions
    function print_error($value, $message) {
        $array = array('error' => $value, 'message' => $message);
        $json = json_encode($array);

        echo $json;
    }

    function check_error($ret, $object) {
        if (!$ret) {
            print_error($object->errono, $object->error);

            exit();
        }
    }

    // Connection definitions
    $hostname = "HOST_NAME";
    $portname = 3306;
    $username = 'MYSQL_USER'; 
    $password = 'MYSQL_PASSWORD';
    $database = 'MYSQL_DATABASE';

    // Create MYSQLi connection
    $mysqli = new mysqli($hostname, $username, $password, $database, $portname);

    if ($mysqli->connect_errno) {
        print_error($mysqli->connect_errno, $mysqli->connect_error);

        exit();
    }

    // Simple REST server
    $method = $_SERVER["REQUEST_METHOD"];
    $override = $_SERVER["HTTP_X_HTTP_METHOD_OVERRIDE"];
    $body = file_get_contents('php://input');

    // Override if X-HTTP-Method-Override is set
    if (mb_strlen($override)) {
        $method = $override;
    }

    // Parse JSON data
    $json = json_decode($body, true);
    $param = $json["param"];

    if (json_last_error()) {
        print_error(1, json_last_error_msg());

        exit();
    }

    if ($method == "POST") {
        switch ($json["id"]) {
            case 0:
                $sql = "SELECT c_discount, c_last, c_credit, w_tax FROM customer, warehouse WHERE w_id = ? AND c_w_id = w_id AND c_d_id = ? AND c_id = ?";

                $query = $mysqli->prepare($sql);
                check_error($query, $mysqli);

                $ret = $query->bind_param('iii', $param[0], $param[1], $param[2]);
                check_error($ret, $query);

                $ret = $query->execute();
                check_error($ret, $query);
            
                break;
            case 1:
                $sql = "SELECT d_next_o_id, d_tax FROM district WHERE d_id = ? AND d_w_id = ? FOR UPDATE";
                $query = $mysqli->prepare($sql);
                check_error($query, $mysqli);

                $ret = $query->bind_param('ii', $param[0], $param[1]);
                check_error($ret, $query);

                $ret = $query->execute();
                check_error($ret, $query);
            
                break;
            case 2:
                $sql = "UPDATE district SET d_next_o_id = ? + 1 WHERE d_id = ? AND d_w_id = ?";
                $query = $mysqli->prepare($sql);
                check_error($query, $mysqli);

                $ret = $query->bind_param('iii', $param[0], $param[1], $param[2]);
                check_error($ret, $query);

                $ret = $query->execute();
                check_error($ret, $query);
            
                break;
            case 3:
                $sql = "INSERT INTO orders (o_id, o_d_id, o_w_id, o_c_id, o_entry_d, o_ol_cnt, o_all_local) VALUES(?, ?, ?, ?, ?, ?, ?)";
                $query = $mysqli->prepare($sql);
                check_error($query, $mysqli);

                $ret = $query->bind_param('iiiisii', $param[0], $param[1], $param[2], $param[3], $param[4], $param[5], $param[6]);
                check_error($ret, $query);

                $ret = $query->execute();
                check_error($ret, $query);
            
                break;
            case 4:
                $sql = "INSERT INTO new_orders (no_o_id, no_d_id, no_w_id) VALUES (?,?,?)";
                $query = $mysqli->prepare($sql);
                check_error($query, $mysqli);

                $ret = $query->bind_param('iii', $param[0], $param[1], $param[2]);
                check_error($ret, $query);

                $ret = $query->execute();
                check_error($ret, $query);
            
                break;
            case 5:
                $sql = "SELECT i_price, i_name, i_data FROM item WHERE i_id = ?";
                $query = $mysqli->prepare($sql);
                check_error($query, $mysqli);

                $ret = $query->bind_param('i', $param[0]);
                check_error($ret, $query);

                $ret = $query->execute();
                check_error($ret, $query);
            
                break;
            case 6:
                $sql = "SELECT s_quantity, s_data, s_dist_01, s_dist_02, s_dist_03, s_dist_04, s_dist_05, s_dist_06, s_dist_07, s_dist_08, s_dist_09, s_dist_10 FROM stock WHERE s_i_id = ? AND s_w_id = ? FOR UPDATE";
                $query = $mysqli->prepare($sql);
                check_error($query, $mysqli);

                $ret = $query->bind_param('ii', $param[0], $param[1]);
                check_error($ret, $query);

                $ret = $query->execute();
                check_error($ret, $query);            
            
                break;
            case 7:
                $sql = "UPDATE stock SET s_quantity = ? WHERE s_i_id = ? AND s_w_id = ?";
                $query = $mysqli->prepare($sql);
                check_error($query, $mysqli);
                
                $ret = $query->bind_param('iii', $param[0], $param[1], $param[2]);
                check_error($ret, $query);

                $ret = $query->execute();
                check_error($ret, $query);            
            
                break;
            case 8:
                $sql = "INSERT INTO order_line (ol_o_id, ol_d_id, ol_w_id, ol_number, ol_i_id, ol_supply_w_id, ol_quantity, ol_amount, ol_dist_info) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)";
                $query = $mysqli->prepare($sql);
                check_error($query, $mysqli);

                $ret = $query->bind_param('iiiiiiiis', $param[0], $param[1], $param[2], $param[3], $param[4], $param[5], $param[6], $param[7], $param[8]);
                check_error($ret, $query);

                $ret = $query->execute();
                check_error($ret, $query);            
            
                break;
            case 9:
                $sql = "UPDATE warehouse SET w_ytd = w_ytd + ? WHERE w_id = ?";
                $query = $mysqli->prepare($sql);
                check_error($query, $mysqli);

                $ret = $query->bind_param('di', $param[0], $param[1]);
                check_error($ret, $query);

                $ret = $query->execute();
                check_error($ret, $query);
            
                break;
            case 10:
                $sql = "SELECT w_street_1, w_street_2, w_city, w_state, w_zip, w_name FROM warehouse WHERE w_id = ?";
                $query = $mysqli->prepare($sql);
                check_error($query, $mysqli);

                $ret = $query->bind_param('i', $param[0]);
                check_error($ret, $query);

                $ret = $query->execute();
                check_error($ret, $query);
            
                break;
            case 11:
                $sql = "UPDATE district SET d_ytd = d_ytd + ? WHERE d_w_id = ? AND d_id = ?";
                $query = $mysqli->prepare($sql);
                check_error($query, $mysqli);

                $ret = $query->bind_param('dii', $param[0], $param[1], $param[2]);
                check_error($ret, $query);

                $ret = $query->execute();
                check_error($ret, $query);            
            
                break;
            case 12:
                $sql = "SELECT d_street_1, d_street_2, d_city, d_state, d_zip, d_name FROM district WHERE d_w_id = ? AND d_id = ?";
                $query = $mysqli->prepare($sql);
                check_error($query, $mysqli);

                $ret = $query->bind_param('ii', $param[0], $param[1]);
                check_error($ret, $query);

                $ret = $query->execute();
                check_error($ret, $query); 
            
                break;
            case 13:
                $sql = "SELECT count(c_id) FROM customer WHERE c_w_id = ? AND c_d_id = ? AND c_last = ?";
                $query = $mysqli->prepare($sql);
                check_error($query, $mysqli);

                $ret = $query->bind_param('iis', $param[0], $param[1], $param[2]);
                check_error($ret, $query);

                $ret = $query->execute();
                check_error($ret, $query);
            
                break;
            case 14:
                $sql = "SELECT c_id FROM customer WHERE c_w_id = ? AND c_d_id = ? AND c_last = ? ORDER BY c_first";
                $query = $mysqli->prepare($sql);
                check_error($query, $mysqli);

                $ret = $query->bind_param('iis', $param[0], $param[1], $param[2]);
                check_error($ret, $query);

                $ret = $query->execute();
                check_error($ret, $query);            
            
                break;
            case 15:
                $sql = "SELECT c_first, c_middle, c_last, c_street_1, c_street_2, c_city, c_state, c_zip, c_phone, c_credit, c_credit_lim, c_discount, c_balance, c_since FROM customer WHERE c_w_id = ? AND c_d_id = ? AND c_id = ? FOR UPDATE";
                $query = $mysqli->prepare($sql);
                check_error($query, $mysqli);

                $ret = $query->bind_param('iii', $param[0], $param[1], $param[2]);
                check_error($ret, $query);

                $ret = $query->execute();
                check_error($ret, $query);
            
                break;
            case 16:
                $sql = "SELECT c_data FROM customer WHERE c_w_id = ? AND c_d_id = ? AND c_id = ?";
                $query = $mysqli->prepare($sql);
                check_error($query, $mysqli);

                $ret = $query->bind_param('iii', $param[0], $param[1], $param[2]);
                check_error($ret, $query);

                $ret = $query->execute();
                check_error($ret, $query);            
            
                break;
            case 17:
                $sql = "UPDATE customer SET c_balance = ?, c_data = ? WHERE c_w_id = ? AND c_d_id = ? AND c_id = ?";
                $query = $mysqli->prepare($sql);
                check_error($query, $mysqli);

                $ret = $query->bind_param('dsiii', $param[0], $param[1], $param[2], $param[3], $param[4]);
                check_error($ret, $query);

                $ret = $query->execute();
                check_error($ret, $query);
            
                break;
            case 18:
                $sql = "UPDATE customer SET c_balance = ? WHERE c_w_id = ? AND c_d_id = ? AND c_id = ?";
                $query = $mysqli->prepare($sql);
                check_error($query, $mysqli);

                $ret = $query->bind_param('diii', $param[0], $param[1], $param[2], $param[3]);
                check_error($ret, $query);

                $ret = $query->execute();
                check_error($ret, $query);
            
                break;
            case 19:
                $sql = "INSERT INTO history(h_c_d_id, h_c_w_id, h_c_id, h_d_id, h_w_id, h_date, h_amount, h_data) VALUES(?, ?, ?, ?, ?, ?, ?, ?)";
                $query = $mysqli->prepare($sql);
                check_error($query, $mysqli);

                $ret = $query->bind_param('iiiiisds', $param[0], $param[1], $param[2], $param[3], $param[4], $param[5], $param[6], $param[7]);
                check_error($ret, $query);

                $ret = $query->execute();
                check_error($ret, $query);
            
                break;
            case 20:
                $sql = "SELECT count(c_id) FROM customer WHERE c_w_id = ? AND c_d_id = ? AND c_last = ?";
                $query = $mysqli->prepare($sql);
                check_error($query, $mysqli);

                $ret = $query->bind_param('iis', $param[0], $param[1], $param[2]);
                check_error($ret, $query);

                $ret = $query->execute();
                check_error($ret, $query);            
            
                break;
            case 21:
                $sql = "SELECT c_balance, c_first, c_middle, c_last FROM customer WHERE c_w_id = ? AND c_d_id = ? AND c_last = ? ORDER BY c_first";
                $query = $mysqli->prepare($sql);
                check_error($query, $mysqli);

                $ret = $query->bind_param('iis', $param[0], $param[1], $param[2]);
                check_error($ret, $query);

                $ret = $query->execute();
                check_error($ret, $query);
            
                break;
            case 22:
                $sql = "SELECT c_balance, c_first, c_middle, c_last FROM customer WHERE c_w_id = ? AND c_d_id = ? AND c_id = ?";
                $query = $mysqli->prepare($sql);
                check_error($query, $mysqli);

                $ret = $query->bind_param('iii', $param[0], $param[1], $param[2]);
                check_error($ret, $query);

                $ret = $query->execute();
                check_error($ret, $query);            
            
                break;
            case 23:
                $sql = "SELECT o_id, o_entry_d, COALESCE(o_carrier_id,0) FROM orders WHERE o_w_id = ? AND o_d_id = ? AND o_c_id = ? AND o_id = (SELECT MAX(o_id) FROM orders WHERE o_w_id = ? AND o_d_id = ? AND o_c_id = ?)";
                $query = $mysqli->prepare($sql);
                check_error($query, $mysqli);

                $ret = $query->bind_param('iiiiii', $param[0], $param[1], $param[2], $param[3], $param[4], $param[5]);
                check_error($ret, $query);

                $ret = $query->execute();
                check_error($ret, $query);
            
                break;
            case 24:
                $sql = "SELECT ol_i_id, ol_supply_w_id, ol_quantity, ol_amount, ol_delivery_d FROM order_line WHERE ol_w_id = ? AND ol_d_id = ? AND ol_o_id = ?";
                $query = $mysqli->prepare($sql);
                check_error($query, $mysqli);

                $ret = $query->bind_param('iii', $param[0], $param[1], $param[2]);
                check_error($ret, $query);

                $ret = $query->execute();
                check_error($ret, $query);
            
                break;
            case 25:
                $sql = "SELECT COALESCE(MIN(no_o_id),0) FROM new_orders WHERE no_d_id = ? AND no_w_id = ?";
                $query = $mysqli->prepare($sql);
                check_error($query, $mysqli);

                $ret = $query->bind_param('ii', $param[0], $param[1]);
                check_error($ret, $query);

                $ret = $query->execute();
                check_error($ret, $query);            
            
                break;
            case 26:
                $sql = "DELETE FROM new_orders WHERE no_o_id = ? AND no_d_id = ? AND no_w_id = ?";
                $query = $mysqli->prepare($sql);
                check_error($query, $mysqli);

                $ret = $query->bind_param('iii', $param[0], $param[1], $param[2]);
                check_error($ret, $query);

                $ret = $query->execute();
                check_error($ret, $query);             
            
                break;
            case 27:
                $sql = "SELECT o_c_id FROM orders WHERE o_id = ? AND o_d_id = ? AND o_w_id = ?";
                $query = $mysqli->prepare($sql);
                check_error($query, $mysqli);

                $ret = $query->bind_param('iii', $param[0], $param[1], $param[2]);
                check_error($ret, $query);

                $ret = $query->execute();
                check_error($ret, $query);
            
                break;
            case 28:
                $sql = "UPDATE orders SET o_carrier_id = ? WHERE o_id = ? AND o_d_id = ? AND o_w_id = ?";
                $query = $mysqli->prepare($sql);
                check_error($query, $mysqli);

                $ret = $query->bind_param('iiii', $param[0], $param[1], $param[2], $param[3]);
                check_error($ret, $query);

                $ret = $query->execute();
                check_error($ret, $query);
            
                break;
            case 29:
                $sql = "UPDATE order_line SET ol_delivery_d = ? WHERE ol_o_id = ? AND ol_d_id = ? AND ol_w_id = ?";
                $query = $mysqli->prepare($sql);
                check_error($query, $mysqli);

                $ret = $query->bind_param('siii', $param[0], $param[1], $param[2], $param[3]);
                check_error($ret, $query);

                $ret = $query->execute();
                check_error($ret, $query);            
            
                break;
            case 30:
                $sql = "SELECT SUM(ol_amount) FROM order_line WHERE ol_o_id = ? AND ol_d_id = ? AND ol_w_id = ?";
                $query = $mysqli->prepare($sql);
                check_error($query, $mysqli);

                $ret = $query->bind_param('iii', $param[0], $param[1], $param[2]);
                check_error($ret, $query);

                $ret = $query->execute();
                check_error($ret, $query);            
            
                break;
            case 31:
                $sql = "UPDATE customer SET c_balance = c_balance + ? , c_delivery_cnt = c_delivery_cnt + 1 WHERE c_id = ? AND c_d_id = ? AND c_w_id = ?";
                $query = $mysqli->prepare($sql);
                check_error($query, $mysqli);

                $ret = $query->bind_param('diii', $param[0], $param[1], $param[2], $param[3]);
                check_error($ret, $query);

                $ret = $query->execute();
                check_error($ret, $query);            
            
                break;
            case 32:
                $sql = "SELECT d_next_o_id FROM district WHERE d_id = ? AND d_w_id = ?";
                $query = $mysqli->prepare($sql);
                check_error($query, $mysqli);

                $ret = $query->bind_param('ii', $param[0], $param[1]);
                check_error($ret, $query);

                $ret = $query->execute();
                check_error($ret, $query);              
            
                break;
            case 33:
                $sql = "SELECT DISTINCT ol_i_id FROM order_line WHERE ol_w_id = ? AND ol_d_id = ? AND ol_o_id < ? AND ol_o_id >= (? - 20)";
                $query = $mysqli->prepare($sql);
                check_error($query, $mysqli);

                $ret = $query->bind_param('iiii', $param[0], $param[1], $param[2], $param[3]);
                check_error($ret, $query);

                $ret = $query->execute();
                check_error($ret, $query);             
            
                break;
            case 34:
                $sql = "SELECT count(*) FROM stock WHERE s_w_id = ? AND s_i_id = ? AND s_quantity < ?";
                $query = $mysqli->prepare($sql);
                check_error($query, $mysqli);

                $ret = $query->bind_param('iii', $param[0], $param[1], $param[2]);
                check_error($ret, $query);

                $ret = $query->execute();
                check_error($ret, $query);    
                
                break;
        }

        // Create response
        $result = $query->get_result();
        
        $response = array(
            "error" => $query->errno,
            "inserted_id" => $query->insert_id,
            "affected_rows" => $query->affected_rows
        );

        // Do we have result?
        if ($result->num_rows > 0) {
            $response["result"] = $result->fetch_all();
        }
        else {
            $response["result"] = array();
        }

        $query->free_result();
        $query->close();

        echo json_encode($response);
    }
    elseif ($method == "OPTIONS") {
        // Assumes CORS preflight
        header('Access-Control-Allow-Origin: *');
        header('Access-Control-Allow-Method: POST');
        header('Access-Control-Allow-Headers: Content-Type, X-HTTP-Method-Override');

        echo "{\"error\":0}";
    }
    else {
        print_error(1, "Unknown HTTP verb");
    }

    // Close MYSQLi connection
    $mysqli->close();
?>
