Add-Type -AssemblyName System.Drawing

$source = 'C:\Users\user\AppData\Local\Temp\codex-clipboard-4fecce9c-ef8a-48c0-aaac-9008349709e0.png'
$project = 'D:\Hsiang\20260601 圖形介面程式設計 ESP32\ESP32\36_epaper'
$header = Join-Path $project 'logo_three_color.h'
$preview = Join-Path $project 'logo_three_color_128.png'

$src = [System.Drawing.Bitmap]::new($source)
$small = [System.Drawing.Bitmap]::new(128, 128)
$graphics = [System.Drawing.Graphics]::FromImage($small)
$graphics.Clear([System.Drawing.Color]::White)
$graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
$graphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
$graphics.DrawImage($src, 0, 0, 128, 128)
$graphics.Dispose()
$src.Dispose()
$small.Save($preview, [System.Drawing.Imaging.ImageFormat]::Png)

$black = [byte[]]::new(128 * 16)
$red = [byte[]]::new(128 * 16)
for ($i = 0; $i -lt $black.Length; $i++) { $black[$i] = 0xFF; $red[$i] = 0xFF }

for ($y = 0; $y -lt 128; $y++) {
  for ($x = 0; $x -lt 128; $x++) {
    $c = $small.GetPixel($x, $y)
    $isRed = ($c.R -gt 115 -and $c.R -gt ($c.G + 45) -and $c.R -gt ($c.B + 45))
    $isBlack = (-not $isRed -and [Math]::Max($c.R, [Math]::Max($c.G, $c.B)) -lt 115)
    $index = $y * 16 + [int]($x / 8)
    $mask = [byte](0x80 -shr ($x % 8))
    if ($isBlack) { $black[$index] = [byte]($black[$index] -band (0xFF -bxor $mask)) }
    elseif ($isRed) { $red[$index] = [byte]($red[$index] -band (0xFF -bxor $mask)) }
  }
}
$small.Dispose()

$lines = [System.Collections.Generic.List[string]]::new()
$lines.Add('#ifndef LOGO_THREE_COLOR_H')
$lines.Add('#define LOGO_THREE_COLOR_H')
$lines.Add('')
$lines.Add('#include <Arduino.h>')
$lines.Add('')
foreach ($name in @('LOGO_BLACK','LOGO_RED')) {
  $data = if ($name -eq 'LOGO_BLACK') { $black } else { $red }
  $lines.Add("const uint8_t $name[128 * 16] PROGMEM = {")
  for ($i = 0; $i -lt $data.Length; $i += 16) {
    $items = for ($j = 0; $j -lt 16; $j++) { ('0x{0:X2}' -f $data[$i + $j]) }
    $suffix = if ($i + 16 -lt $data.Length) { ',' } else { '' }
    $lines.Add(('  ' + ($items -join ', ') + $suffix))
  }
  $lines.Add('};')
  $lines.Add('')
}
$lines.Add('#endif')
[System.IO.File]::WriteAllLines($header, $lines, [System.Text.Encoding]::ASCII)
Write-Output "Generated $header"
Write-Output "Preview $preview"
