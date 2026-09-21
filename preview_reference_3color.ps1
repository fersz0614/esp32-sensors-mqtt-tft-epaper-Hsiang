Add-Type -AssemblyName System.Drawing

$source = 'C:\Users\user\AppData\Local\Temp\codex-clipboard-789f7bad-c4d0-4deb-a229-17a1c7cb8e43.png'
$output = Join-Path $PSScriptRoot 'ESP32\37_epaper_dht\37_epaper_dht_reference_preview.png'
$src = [Drawing.Bitmap]::new($source)
$dst = [Drawing.Bitmap]::new(128,296,[Drawing.Imaging.PixelFormat]::Format24bppRgb)
$g = [Drawing.Graphics]::FromImage($dst)
$g.Clear([System.Drawing.Color]::FromArgb(255,255,255))
$g.InterpolationMode = [Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
$g.PixelOffsetMode = [Drawing.Drawing2D.PixelOffsetMode]::HighQuality
$g.DrawImage($src,0,0,128,296)
$g.Dispose(); $src.Dispose()

$red = [System.Drawing.Color]::FromArgb(220,0,0)
for($y=0;$y -lt 296;$y++){
  for($x=0;$x -lt 128;$x++){
    $c=$dst.GetPixel($x,$y)
    # 先判斷紅色，再以灰階門檻判斷黑白，避免抗鋸齒像素產生紅色雜點
    $lum=(0.299*$c.R)+(0.587*$c.G)+(0.114*$c.B)
    if($c.R -gt 90 -and $c.R -gt ($c.G * 1.35) -and $c.R -gt ($c.B * 1.35)){$dst.SetPixel($x,$y,$red)} elseif($lum -lt 145){$dst.SetPixel($x,$y,[System.Drawing.Color]::FromArgb(0,0,0))} else {$dst.SetPixel($x,$y,[System.Drawing.Color]::FromArgb(255,255,255))}
  }
}
$dst.Save($output,[Drawing.Imaging.ImageFormat]::Png)
$dst.Dispose()
Write-Output $output
