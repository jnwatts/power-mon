<?php

if (ini_get('register_globals')) {
    die();
}
if (!defined('API')) {
    die();
}
if (API !== true) {
    die();
}

$config = [
    'db_conn' => 'mysql:host=DBHOST;dbname=DBNAME',
    'db_user' => 'DBUSER',
    'db_pass' => 'DBPASS',
];

?>
