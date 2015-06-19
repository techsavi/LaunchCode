<html>
	<head>
		<title>Fireworks!</title>
		
		<style type="text/css">
		body {
			color:#000000;
			background-color: #FFFFFF;
			background-image: url('images/USA-Flag-Waving.png');
		}
		
		.fw {
			display: inline-block;
			text-align: center;
			float: left;
			width: 100px;
			height: 120px;
		}
		
		.header {
			width: 100%;
			height: 200px;		
		}

		.content {
			position: absolute;
			display: inline-block;
			text-align: center;
			/*margin: 0 auto;
			top: 240px;
			bottom: 0px;*/
			width: 100%;
			height: 100%;
			overflow-y: scroll;		
		}
		
		.select {
			width: 64px;
			height: 64px;		
		}
		
		.current {
			color: black;
			font-weight: bolder;
			font-size: large;		
		}
		
		.status-good {
			border-color: green;
			border-width: 3;
			border-style: solid;
		}

		.status-bad {
			border-color: red;
			border-width: 3;
			border-style: dashed;
		}		

		.status-launching {
			border-color: yellow;
			border-width: 3;
			border-style: dotted;
		}

		.status-launched {
			border-color: gray;
			border-width: 3;
			border-style: double;
		}
		
		.ident {
			position: absolute;
			font-size: 24px;
			font-weight: bold;
			text-shadow: black 0.1em 0.1em 0.2em;
			color: white;
			padding-left: 3px;
			padding-top: 1px;
		}

		.hidden {
			display: none;		
		}
		</style>
	</head>
	<body>
		<script type="text/javascript">
			var ua = window.navigator.userAgent;
			var msie = ua.indexOf("MSIE ");
			var CURID = -1;
			
			function sendCommand() {
				if (CURID == -1)
					return;
					
				var xmlHttp = new XMLHttpRequest();
				
				xmlHttp.open("GET", "serial.php?req=[L" + CURID + "]", true);
				xmlHttp.send(null);
				
				//alert("Launched! " + CURID);
				
				CURID = -1;
			}
			
			function getStatus() {
				var xhr = new XMLHttpRequest();
				
				xhr.onreadystatechange = function () {
					if (xhr.readyState == 4) {
						var vals = xhr.responseText.split(",");
						
						for (x = 0; x < vals.length / 2; x++) {
							var status;
							
							if (parseInt(vals[x * 2 + 1]) < 100)
								status = "bad";
							else
								status = "good";
								
							updateStatus(vals[x * 2], status, vals[x * 2 + 1]);
						}
						
					}
				}
				
				xhr.open("GET", "serial.php?req=[S]", true);
				xhr.send(null);
				setTimeout(getStatus, 1000);
			}
			
			function setCurrent(id) {
				var curImageElem = document.getElementById("currentImage");
				var curNameElem = document.getElementById("currentName");
				var curStatusElem = document.getElementById("currentStatus");
				var curOhmsElem = document.getElementById("currentOhms");
				var launchButtonElem = document.getElementById("launchButton");
				var div = document.getElementById("fw" + id);
				
				curImageElem.src = div.childNodes[0].childNodes[1].src;
				curNameElem.innerHTML = div.childNodes[0].childNodes[3].innerHTML;
				curStatusElem.innerHTML = div.childNodes[0].childNodes[4].innerHTML;
				curOhmsElem.innerHTML = "(" + div.childNodes[0].childNodes[5].innerHTML + ")";
				
				CURID = id;				
			}
			
			function updateStatus(id, status, ohms) {
				var div = document.getElementById("fw" + id);
				
				if (div) {
					div.childNodes[0].className = "status-" + status;
					div.childNodes[0].childNodes[4].innerHTML = status;
					div.childNodes[0].childNodes[5].innerHTML = ohms;
				}
			}
			
			function init() {
				resizeContent();
				//getStatus();
				
				setTimeout(getStatus, 1000);
			}
			
			function resizeContent() {
				var div = document.getElementById("content");
				//var div2 = document.getElementById("currentOhms");
				//div2.innerHTML = window.innerHeight;
				
				if (msie > 0)
					div.style.height = (document.body.clientHeight - 240) + "px";
				else
					div.style.height = (window.innerHeight - 240) + "px";
			}
			
			window.onload = init;
			window.onresize = function () {
				setTimeout(resizeContent, 250);
			}
		</script>
		<!--<h1 style="text-align: center;">Fireworks!</h1>-->
		<div class="header" id="header">
			<table style="width: 100%;">
				<tr>
					<td style="text-align: center; valign: center; width: 50%;">
						<table>
							<tr>
								<td>
									<img id="currentImage" style="width: 160px; height: 160px;" src="images/unknown.png" alt="">
								</td>
								<td>
									<table>
										<tr>
											<td style="text-align: left;">
												<span id="currentName" class="current">{name goes here}</span>
											</td>
										</tr>
										<tr>
											<td style="text-align: center;">
												<span id="currentStatus">{status goes here}</span>
											</td>
										</tr>
										<tr>
											<td style="text-align: center;">
												<span id="currentOhms">{ohms goes here}</span>
											</td>
										</tr>	
									</table>
								</td>							
							</tr>	
						</table>
					</td>
					<td style="text-align: center; valign: center; width: 50%;" rowspan="3">
						<img id="launchButton" src="images/launch.png" onclick="javascript:sendCommand();"/>					
					</td>
				</tr>
			</table>

		</div>
		<hr/>
		<div class="content" id="content">
		<?php
			$file_handle = fopen("fw2014.txt", "r");
			
			while(!feof($file_handle)) {
				$line = fgets($file_handle);
				
				list($num, $id, $name, $image) = explode(",", $line);
				
				if (strlen($name) > 0)
					echo generateDiv($num, $id, $name, $image);
			}
			
			fclose($file_handle);
			
			function generateDiv($num, $id, $name, $image) {
				$div = '<div class="fw" id="fw' . $id . '">';
				$div .= '<div>';
				$div .= '<span class="ident">' . $num . '</span>';
				$div .= '<img class="select" id="fwimg' . $id . '" src="images/' . $image . '" onclick="javascript:setCurrent(\'' . $id . '\')" />';
				$div .= '<br/>';				
				$div .= '<span id="fwname' . $id . '">' . $name . '</span>';
				$div .= '<div class="hidden" id="fw' . $id . '-status"></div>';
				$div .= '<div class="hidden" id="fw' . $id . '-ohms"></div>';
				$div .= '</div>';
				$div .= '</div>';
				
				return $div;		
			}
		?>
		</div>
	</body>
</html>
