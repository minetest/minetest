<!DOCTYPE html>
<?php
	$stopserver = $_REQUEST['stopserver'];
	$string = "";
	if ($stopserver)
	{
		exec("killall minetestserver", $output, $result);
		if ($result == 0)
		{
			$string = "Server stopped!";
		}
		else
		{
			$string = "Server failed to stop:<br /><code>";
			foreach ($output as $i)
			{
				$string = $string . $i;
			}
			$string = $string . "</code>";
		}
	}
?>
<html>
<head>
	<title>Blockscape Webadmin</title>
</head>
<body>
	<a href="?stopserver=true" title="Stop server">Stop server</a>
	<p><?php echo $string; ?></p>
</body>
</html>
