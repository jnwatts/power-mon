<?php
define('API', true);
include_once('./config.php');

$db = new PDO($config['db_conn'], $config['db_user'], $config['db_pass']);
$res = $db->query('SELECT c.name, d.ch_id, d.timestamp, d.voltage, d.current, d.power FROM data AS d, channels AS c WHERE d.ch_id = c.ch_id AND timestamp = (SELECT MAX(timestamp) FROM data)');
$channels = [];
foreach ($res->fetchAll(PDO::FETCH_ASSOC) as $row) {
    $channels[$row['name']] = $row;
}

header('Content-Type: application/json');
print(json_encode($channels));

?>