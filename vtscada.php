#!/usr/bin/php

<?php
// ****************************************************************************
	// Build VTScada Configs from Dnp3Tags.csv
// ****************************************************************************

///$tagFileName= "/dfs/hypertac/bin/drivers/dnp3tags.csv";
$tagFileName= "./dnp3tags.csv";
$digitalInputFileName = "./dfsDigitalInput.csv";
$tag = "";

if((@$iFile = fopen($tagFileName,"r"))){
	if(($oFile = fopen($digitalInputFileName,"w+"))){
		while($line = fgets($iFile)){
			$line = str_replace("\n", "", $line);
			if(substr($line,0,1) != "#"){	// ignore comments

				if(list($type, $index, $tag, $name) = explode(",", $line)){
					echo"type: $type index: $index tag: $tag station: $station name: $name line: $line\n";///
					if(list($station, $modPnt) = sscanf($tag, "%d%s")){
				
						// Build DigitalInput
						if($type == "DI"){
							if((intval($index) < 49) && ($station == 1019)){	/// reduce i/o count for lite version
								fputs($oFile, ",DFS:$station\\\\$name($modPnt),,,*Driver,1/0/$index:NS,,0,1,,,0,0,,,0,,*Style Settings,1,,$name,\n");
							}
						}
					}
				 }
			}
		}
		fclose($oFile);
	}
	else{
		echo"Could not open output file(s)\n";
	}
	fclose($iFile);
}
else{
	echo"No tag file found\n";
}
?>

