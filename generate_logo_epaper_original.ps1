Add-Type -AssemblyName System.Drawing
$source = 'C:\Users\user\AppData\Local\Temp\codex-clipboard-a07db18a-2f54-43be-8cc6-fe763b5b676a.png'
$project = 'D:\Hsiang\20260601 圖形介面程式設計 ESP32\ESP32\36_epaper'
$header = Join-Path $project 'logo_three_color.h'
$preview = Join-Path $project 'logo_three_color_original_128.png'
$src = [System.Drawing.Bitmap]::new($source)
$small = [System.Drawing.Bitmap]::new(128,128)
$g = [System.Drawing.Graphics]::FromImage($small)
$g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::NearestNeighbor
$g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::Half
$g.DrawImage($src,0,0,128,128); $g.Dispose(); $src.Dispose()
$black=[byte[]]::new(2048); $red=[byte[]]::new(2048)
for($i=0;$i -lt 2048;$i++){ $black[$i]=0xFF; $red[$i]=0xFF }
$previewBmp=[System.Drawing.Bitmap]::new(128,128)
for($y=0;$y -lt 128;$y++){ for($x=0;$x -lt 128;$x++){
  $c=$small.GetPixel($x,$y); $max=[Math]::Max($c.R,[Math]::Max($c.G,$c.B)); $min=[Math]::Min($c.R,[Math]::Min($c.G,$c.B))
  $isRed=($c.R -gt 115 -and $c.R -gt ($c.G+35) -and $c.R -gt ($c.B+35))
  $isBlack=(-not $isRed -and $max -lt 125)
  $p=0; if($isBlack){$p=1} elseif($isRed){$p=2}
  $color=if($p -eq 1){[System.Drawing.Color]::Black}elseif($p -eq 2){[System.Drawing.Color]::Red}else{[System.Drawing.Color]::White}
  $previewBmp.SetPixel($x,$y,$color)
  $idx=($y*16)+[int]([Math]::Floor($x/8)); $mask=[byte](0x80 -shr ($x%8))
  if($p -eq 1){$black[$idx]=[byte]($black[$idx]-band (0xFF -bxor $mask))} elseif($p -eq 2){$red[$idx]=[byte]($red[$idx]-band (0xFF -bxor $mask))}
} }
$small.Dispose(); $previewBmp.Save($preview,[System.Drawing.Imaging.ImageFormat]::Png); $previewBmp.Dispose()
$lines=[System.Collections.Generic.List[string]]::new(); $lines.Add('#ifndef LOGO_THREE_COLOR_H'); $lines.Add('#define LOGO_THREE_COLOR_H'); $lines.Add(''); $lines.Add('#include <Arduino.h>'); $lines.Add('')
foreach($name in @('LOGO_BLACK','LOGO_RED')){ $data=if($name -eq 'LOGO_BLACK'){$black}else{$red}; $lines.Add("const uint8_t $name[128 * 16] PROGMEM = {"); for($i=0;$i -lt 2048;$i+=16){$items=for($j=0;$j -lt 16;$j++){('0x{0:X2}' -f $data[$i+$j])};$comma=if($i+16 -lt 2048){','}else{''};$lines.Add(('  '+($items -join ', ')+$comma))};$lines.Add('};');$lines.Add('')}
$lines.Add('#endif'); [System.IO.File]::WriteAllLines($header,$lines,[System.Text.Encoding]::ASCII); Write-Output 'Original-detail three-color bitmap generated'
