<?php
include 'PhpSerial.php';

//$txtemplate = [0x7e, 0x00, 0x12, 0x10, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xfe, 0x00, 0x00, 0x74, 0x65, 0x73, 0x74, 0x33];
$XB1S = pack("CCCCCCCC", 0x00, 0x13, 0xa2, 0x00, 0x40, 0xa1, 0x3f, 0x49);
$XB1M = pack("CC", 0xc3, 0x1b);
$XB2S = pack("CCCCCCCC", 0x00, 0x13, 0xa2, 0x00, 0x40, 0xa1, 0x3e, 0xfd);
$XB2M = pack("CC", 0xfe, 0x72);
$XB3S = pack("CCCCCCCC", 0x00, 0x13, 0xa2, 0x00, 0x40, 0xa1, 0x40, 0x16);
$XB3M = pack("CC", 0x66, 0xcf);
$XB4S = pack("CCCCCCCC", 0x00, 0x13, 0xa2, 0x00, 0x40, 0xa1, 0x3f, 0x43);
$XB4M = pack("CC", 0x61, 0xef);


$outstr = isset($_GET['req']) ? $_GET['req'] : "[S]";
$reqlen = strlen($outstr);
$reqlen = $reqlen + 14;

// which index
$lbracket = strpos($outstr, "[");
$rbracket = strpos($outstr, "]", $lbracket);
$cmdstr = substr($outstr, $lbracket + 1, $rbracket - $lbracket - 1);

// Let's start the class
$serial = new PhpSerial;

// First we must specify the device. This works on both linux and windows (if
// your linux serial device is /dev/ttyS0 for COM1, etc)
$serial->deviceSet("/dev/ttyAMA0");

// We can change the baud rate, parity, length, stop bits, flow control
$serial->confBaudRate(9600);
$serial->confParity("none");
$serial->confCharacterLength(8);
$serial->confStopBits(1);
$serial->confFlowControl("none");

// Then we need to open it
do {
	$serial->deviceOpen();
} while (!$serial->_ckOpened());

$serial->_exec("stty -F /dev/ttyAMA0 -isig", $out);
$serial->_exec("stty -F /dev/ttyAMA0 -icanon", $out);

// range of where to send
$destbegin = 0;
$destend = 1;

// for launch command map to a device and index, reformat message
$cmdcode = $cmdstr[0];
if ($cmdcode === 'L')
{
	$cmdval = intval(substr($cmdstr, 1));
	$destend = $destbegin = (int) ($cmdval / 20);
	$outstr = "[L" . strval($cmdval % 20) . "]"; 
}

$response = "";

for ($d = $destbegin; $d <= $destend; $d++)
{
	$out = pack("C", 0x7e);
	$out .= pack("n", $reqlen);
	$out .= pack("CC", 0x10, 0x01);
	switch ($d) {
		case 0: $out .= $XB1S . $XB1M; break;
		case 1: $out .= $XB2S . $XB2M; break;
		case 2: $out .= $XB3S . $XB3M; break;
		case 3: $out .= $XB4S . $XB4M; break;
	}
	$out .= pack("n", 0x00);
	$out .= $outstr;

	$sum = 0;
	for ($i = 3; $i < $reqlen + 3; $i++)
	{
		$sum += ord($out[$i]);
	}
	$sum = 0xff - ($sum & 0xff);
	$out .= pack("C", $sum);

	$serial->sendMessage($out);
//echo "Sending " . bin2hex($out) . "\n";
	// wait for reply
	$inbuf = '';
	$done = false;
	do {
		$datain = $serial->readPort(); 
		if ($datain) $inbuf .= $datain;
//if ($datain) echo bin2hex($datain) . " ";
		$len = strlen($inbuf);
		if ($len == 0) continue;
		if (ord($inbuf[0]) != 0x7e) {$inbuf = substr($inbuf, 1); continue;} // bad framing
		if ($len < 3) continue; // need at least frame header and size
		$packetsize = ord($inbuf[1]) * 0x100 + ord($inbuf[2]);
		if ($len < $packetsize + 4) continue; // full message not here yet
		if (ord($inbuf[3]) == 0x8b) { // transmit ack
			if (ord($inbuf[8]) != 0) {
//				echo "Transmit failed code " . ord($inbuf[8]) . "\n";
				break;
			}
			$inbuf = substr($inbuf, $packetsize + 4);
			continue;
		}
		if (ord($inbuf[3]) == 0x90) { // receiving incoming transmission
			$indata = substr($inbuf, 15, $packetsize - 12);
			
			if ($cmdcode === 'S')
			{
				$vals = explode(",", $indata);
				for ($i = 0; $i < count($vals); $i += 2)
					$vals[$i] = $vals[$i] + 20 * $d;
				$indata = implode(",", $vals);
			}			
			$response .= $indata; // main response
			$inbuf = substr($inbuf, $packetsize + 4);
			if ($d != $destend) $response .= ",";
			$done = true; // got our response
		}
	} while (!$done);
}
//echo bin2hex($instr);
echo $response;

$serial->deviceClose();

?>