Add-Type -AssemblyName System.Drawing

$source = 'C:\Users\user\AppData\Local\Temp\codex-clipboard-4fecce9c-ef8a-48c0-aaac-9008349709e0.png'
$project = 'D:\Hsiang\20260601 圖形介面程式設計 ESP32\ESP32\36_epaper'
$header = Join-Path $project 'logo_three_color.h'
$preview = Join-Path $project 'logo_three_color_clean_128.png'

$src = [System.Drawing.Bitmap]::new($source)
$small = [System.Drawing.Bitmap]::new(128, 128)
$g = [System.Drawing.Graphics]::FromImage($small)
$g.Clear([System.Drawing.Color]::White)
$g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::NearestNeighbor
$g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::Half
$g.DrawImage($src, 0, 0, 128, 128)
$g.Dispose(); $src.Dispose()

$kind = [byte[]]::new(128 * 128)
for ($y = 0; $y -lt 128; $y++) {
  for ($x = 0; $x -lt 128; $x++) {
    $c = $small.GetPixel($x, $y)
    $max = [Math]::Max($c.R, [Math]::Max($c.G, $c.B))
    $min = [Math]::Min($c.R, [Math]::Min($c.G, $c.B))
    $redLike = ($c.R -gt 120 -and $c.R -gt ($c.G + 40) -and $c.R -gt ($c.B + 40))
    if ($redLike) { $kind[$y * 128 + $x] = 2 }
    elseif ($max -lt 105) { $kind[$y * 128 + $x] = 1 }
    else { $kind[$y * 128 + $x] = 0 }
  }
}

# Remove isolated anti-alias pixels while preserving intentional bold contours.
$clean = [byte[]]$kind.Clone()
for ($y = 1; $y -lt 127; $y++) {
  for ($x = 1; $x -lt 127; $x++) {
    $p = $kind[$y * 128 + $x]
    if ($p -eq 0) { continue }
    $same = 0
    for ($dy = -1; $dy -le 1; $dy++) {
      for ($dx = -1; $dx -le 1; $dx++) {
        if ($dx -eq 0 -and $dy -eq 0) { continue }
        if ($kind[($y + $dy) * 128 + ($x + $dx)] -eq $p) { $same++ }
      }
    }
    if ($same -lt 2) { $clean[$y * 128 + $x] = 0 }
  }
}
$small.Dispose()

$black = [byte[]]::new(128 * 16)
$red = [byte[]]::new(128 * 16)
for ($i = 0; $i -lt 2048; $i++) { $black[$i] = 0xFF; $red[$i] = 0xFF }
for ($y = 0; $y -lt 128; $y++) {
  for ($x = 0; $x -lt 128; $x++) {
    $index = $y * 16 + [int]($x / 8)
    $mask = [byte](0x80 -shr ($x % 8))
    $p = $clean[$y * 128 + $x]
    if ($p -eq 1) { $black[$index] = [byte]($black[$index] -band (0xFF -bxor $mask)) }
    elseif ($p -eq 2) { $red[$index] = [byte]($red[$index] -band (0xFF -bxor $mask)) }
  }
}

$out = [System.Collections.Generic.List[string]]::new()
$out.Add('#ifndef LOGO_THREE_COLOR_H'); $out.Add('#define LOGO_THREE_COLOR_H'); $out.Add('')
$out.Add('#include <Arduino.h>'); $out.Add('')
foreach ($name in @('LOGO_BLACK','LOGO_RED')) {
  $data = if ($name -eq 'LOGO_BLACK') { $black } else { $red }
  $out.Add("const uint8_t $name[128 * 16] PROGMEM = {")
  for ($i = 0; $i -lt $data.Length; $i += 16) {
    $items = for ($j = 0; $j -lt 16; $j++) { ('0x{0:X2}' -f $data[$i + $j]) }
    $comma = if ($i + 16 -lt $data.Length) { ',' } else { '' }
    $out.Add(('  ' + ($items -join ', ') + $comma))
  }
  $out.Add('};'); $out.Add('')
}
$out.Add('#endif')
[System.IO.File]::WriteAllLines($header, $out, [System.Text.Encoding]::ASCII)
$small2 = [System.Drawing.Bitmap]::new(128, 128)
for ($y = 0; $y -lt 128; $y++) { for ($x = 0; $x -lt 128; $x++) {
  $p = $clean[$y * 128 + $x]
  $color = if ($p -eq 1) { [System.Drawing.Color]::Black } elseif ($p -eq 2) { [System.Drawing.Color]::Red } else { [System.Drawing.Color]::White }
  $small2.SetPixel($x, $y, $color)
} }
$small2.Save($preview, [System.Drawing.Imaging.ImageFormat]::Png); $small2.Dispose()
Write-Output "Generated clean three-color bitmap and header"
