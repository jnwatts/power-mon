<?php
define('API', true);
include_once('./config.php');

$min_since = (time() - 36*60*60)*1000.0;
$since = $min_since;
if (isset($_REQUEST['since'])) {
    $since = floatval($_REQUEST['since']);
}
if ($since < $min_since) {
    $since = $min_since;
}
$ch_ids = [1,2];
if (isset($_REQUEST['channels'])) {
    $ch_ids = [];
    $tmp = explode(",", $_REQUEST['channels']);
    foreach ($tmp as $val) {
        $ch_ids[] = intval($val);
    }
}

$db = new PDO($config['db_conn'], $config['db_user'], $config['db_pass']);
$sql = 'SELECT d.ch_id, d.timestamp, d.voltage, d.current, d.power FROM data AS d WHERE timestamp > :since AND d.ch_id in ('.implode(',',$ch_ids).') ORDER BY d.timestamp DESC';
$stmt = $db->prepare($sql);
$stmt->bindParam(':since', $since, PDO::PARAM_INT);
if (!$stmt->execute()) {
    die();
}
$channels = [];
foreach ($stmt->fetchAll(PDO::FETCH_ASSOC) as $row) {
    $channels[$row['ch_id']]['data'][] = $row;
}
foreach ($ch_ids as $ch_id) {
    $c = strval($ch_id);
    if (!array_key_exists($c, $channels)) {
        continue;
    }
    $channels[$c]['count'] = count($channels[$c]['data']);
}

header('Content-Type: application/json');
print(json_encode($channels));

?>