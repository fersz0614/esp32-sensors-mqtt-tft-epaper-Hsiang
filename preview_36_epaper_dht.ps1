Add-Type -AssemblyName System.Drawing
$path=Join-Path $PSScriptRoot 'ESP32\37_epaper_dht\37_epaper_dht_preview.png'
$bmp=[Drawing.Bitmap]::new(128,296); $g=[Drawing.Graphics]::FromImage($bmp); $g.Clear([Drawing.Color]::White)
$red=[Drawing.Color]::FromArgb(220,0,0); $redBrush=[Drawing.SolidBrush]::new($red); $black=[Drawing.Color]::Black
$fontTitle=[Drawing.Font]::new('Arial',8,[Drawing.FontStyle]::Bold); $fontLabel=[Drawing.Font]::new('Arial',10,[Drawing.FontStyle]::Bold); $fontValue=[Drawing.Font]::new('Arial',25,[Drawing.FontStyle]::Bold); $fontUnit=[Drawing.Font]::new('Arial',8,[Drawing.FontStyle]::Bold); $fontFoot=[Drawing.Font]::new('Arial',7,[Drawing.FontStyle]::Bold)
$g.FillRectangle([Drawing.Brushes]::Red,0,0,128,29); $g.DrawString('ENV DATA',$fontTitle,[Drawing.Brushes]::White,8,7)
function Thermometer($y){
  $g.FillRectangle([Drawing.Brushes]::Black,21,$y+10,9,30)
  $g.FillRectangle([Drawing.Brushes]::White,24,$y+13,3,25)
  $g.FillEllipse([Drawing.Brushes]::Black,15,$y+34,21,21)
  $g.FillEllipse([Drawing.Brushes]::White,19,$y+38,13,13)
  $g.FillRectangle([Drawing.Brushes]::Red,25,$y+17,3,21)
  $g.FillEllipse($redBrush,22,$y+41,7,7)
}
function Drop($y){
  $g.FillRectangle([Drawing.Brushes]::Black,23,$y+13,8,24)
  $g.FillEllipse([Drawing.Brushes]::Black,14,$y+29,26,26)
  $g.FillRectangle([Drawing.Brushes]::White,26,$y+17,3,17)
  $g.FillEllipse([Drawing.Brushes]::White,19,$y+34,16,16)
  $g.FillRectangle([Drawing.Brushes]::Red,18,$y+44,18,6)
}
function Sun($y){
  $g.FillEllipse([Drawing.Brushes]::Black,16,$y+23,20,20)
  $g.FillEllipse([Drawing.Brushes]::White,20,$y+27,12,12)
  $g.FillEllipse($redBrush,23,$y+30,6,6)
  $g.DrawLine([Drawing.Pens]::Black,26,$y+11,26,$y+19); $g.DrawLine([Drawing.Pens]::Black,26,$y+47,26,$y+55)
  $g.DrawLine([Drawing.Pens]::Black,7,$y+33,15,$y+33); $g.DrawLine([Drawing.Pens]::Black,37,$y+33,45,$y+33)
}
function Card($y,$label,$value,$unit,$icon){$g.DrawRectangle([Drawing.Pens]::Black,6,$y,115,67);$g.FillRectangle([Drawing.Brushes]::Red,6,$y,9,68);if($icon -eq 1){Thermometer $y}elseif($icon -eq 2){Drop $y}else{Sun $y};$g.DrawString($label,$fontLabel,[Drawing.Brushes]::Black,52,$y+6);$vfont=$fontValue;if($value.Length -gt 2){$vfont=[Drawing.Font]::new('Arial',21,[Drawing.FontStyle]::Bold)};$g.DrawString($value,$vfont,[Drawing.Brushes]::Black,52,$y+23);$g.DrawString($unit,$fontUnit,$redBrush,103,$y+40)}
Card 34 'TEMP' '25' 'C' 1; Card 108 'HUMI' '60' '%' 2; Card 182 'LIGHT' '75' '%' 3; $g.DrawString('LAST UPDATE',$fontFoot,[Drawing.Brushes]::Black,8,259); $g.DrawString('60S AGO',$fontFoot,[Drawing.Brushes]::Black,8,272)
$g.Dispose();$bmp.Save($path,[Drawing.Imaging.ImageFormat]::Png);$bmp.Dispose();Write-Output $path
