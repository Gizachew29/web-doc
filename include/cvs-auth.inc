<?php
/**
 * +----------------------------------------------------------------------+
 * | PHP Documentation Site Source Code                                   |
 * +----------------------------------------------------------------------+
 * | Copyright (c) 1997-2005 The PHP Group                                |
 * +----------------------------------------------------------------------+
 * | This source file is subject to version 3.0 of the PHP license,       |
 * | that is bundled with this package in the file LICENSE, and is        |
 * | available at through the world-wide-web at                           |
 * | http://www.php.net/license/3_0.txt.                                  |
 * | If you did not receive a copy of the PHP license and are unable to   |
 * | obtain it through the world-wide-web, please send a note to          |
 * | license@php.net so we can mail you a copy immediately.               |
 * +----------------------------------------------------------------------+
 * | Authors: Sara Golemon <pollita@php.net>                              |
 * |          Wez Furlong <wez@php.net>                                   |
 * |          Jacques Marneweck <jacques@php.net>                         |
 * +----------------------------------------------------------------------+
 *
 * $Id$
 */

function auth_vs_cvs($name, $pass)
{
	$cvs_encode = array(
		32, 120,  53,  35,  36, 109,  72, 108,
		70,  64,  76,  67, 116,  74,  68,  87,
		111,  52,  75, 119,  49,  34,  82,  81,
		95,  65, 112,  86, 118, 110, 122, 105,
		64,  57,  83,  43,  46, 102,  40,  89,
		38, 103,  45,  50,  42, 123,  91,  35,
		125,  55,  54,  66, 124, 126,  59,  47,
		92,  71, 115,  91,  92,  93,  94,  56,
		96, 121, 117, 104, 101, 100,  69,  73,
		99,  63,  94,  93,  39,  37,  61,  48,
		58, 113,  32,  90,  44,  98,  60,  51,
		33,  97,  62, 123, 124, 125, 126, 127 
	);

	$cvsinfo = array('host'=>'cvs.php.net', 'port'=>2401, 'repos'=>'/repository');

	/* encode the password for cvs */
	$encpwd = 'A';
	$l = strlen($pass);
	for ($i = 0; $i < $l; $i++) {
		$o = ord($pass{$i});
		if ($o >= 32 && $o <= 127) {
			$encpwd .= chr($cvs_encode[$o - 32]);
		} else {
			$encpwd .= $pass{$i};
		}
	}

	$cvs = fsockopen($cvsinfo['host'], $cvsinfo['port'], $errno, $errstr);
	if (!$cvs)
		return $errstr;
	
	$pkt = "BEGIN AUTH REQUEST\n$cvsinfo[repos]\n$name\n$encpwd\nEND AUTH REQUEST\n";
	
	fwrite($cvs, $pkt);
	$response = fgets($cvs);
	fclose($cvs);

	return 0 == strncmp($response, 'I LOVE YOU', 10) ? true : false;
}

/**
 * Check local credential cache before sending extra queries
 * to cvs.php.net
 */
function auth_vs_local ($user, $pass) {
	$res = mysql_query ("SELECT username, password, ctime FROM users WHERE username='" . addslashes($user) . "'") or die ("db query failed");
	if (!mysql_num_rows($res)) {
		return false;
	}

	$row = mysql_fetch_assoc($res);
	if (!is_array($row)) {
		return false;
	}

	if (strtotime($row['ctime']) + 60*60*24 < time()) {
		return 'cache_expire';
	}

	if (crypt($pass, $row['password']) == $row['password']) {
		return true;
	}

	return false;
}

function verify_password($user, $pass) {
	$db = mysql_connect (ini_get('mysql.default_host'), ini_get('mysql.default_user'), ini_get('mysql.default_password')) or die ("cannot connect to db server");
	mysql_select_db ("doc_php_net", $db) or die ("select db failed.");

	/**
	 * You cannot auth as our cvsread@php.net pseudo user
	 */
	if ($user == "cvsread") {
		return false;
	}

	if (empty($pass)) {
		return false;
	}

	$_ret = auth_vs_local($user, $pass);
	if ($_ret === true) {
		return true;
	}

	$_ret = auth_vs_cvs($user, $pass);

	if ($_ret === true) {
		$res = mysql_query ("SELECT username FROM users WHERE username='" . addslashes($user) . "'");
		if (mysql_num_rows($res)) {
			mysql_query ("UPDATE users SET password='" . crypt($pass) . "', ctime=NOW() WHERE username='" . addslashes($user) . "'") or die ("db query failed.");
		} else {
			mysql_query ("INSERT INTO users (username, password, date_added, ctime) VALUES ('" . addslashes($user) . "', '" . crypt($pass) . "', NOW(), NOW())") or die ("db query failed.");
		}
	}

	return ($_ret);
}

/* vim: set noet ts=4 sw=4 ft=php: : */