
Add-Type -AssemblyName System.Drawing

$width = 256
$height = 256
$outputPath = "$PSScriptRoot\app_icon_candidate.png"

$bmp = New-Object System.Drawing.Bitmap $width, $height
$graphics = [System.Drawing.Graphics]::FromImage($bmp)

$graphics.Clear([System.Drawing.Color]::White)
$brush = [System.Drawing.SolidBrush]::new([System.Drawing.Color]::SteelBlue)
$graphics.FillEllipse($brush, 20, 20, 216, 216)

$family = New-Object System.Drawing.FontFamily "Arial"
$style = [System.Drawing.FontStyle]::Bold
$size = [float]48
$font = New-Object System.Drawing.Font($family, $size, $style, [System.Drawing.GraphicsUnit]::Point)

$textBrush = [System.Drawing.SolidBrush]::new([System.Drawing.Color]::White)
$format = New-Object System.Drawing.StringFormat
$format.Alignment = [System.Drawing.StringAlignment]::Center
$format.LineAlignment = [System.Drawing.StringAlignment]::Center

$graphics.DrawString("App", $font, $textBrush, 128, 90, $format)
$graphics.DrawString("Icon", $font, $textBrush, 128, 150, $format)

$bmp.Save($outputPath, [System.Drawing.Imaging.ImageFormat]::Png)

$graphics.Dispose()
$bmp.Dispose()
$brush.Dispose()
$textBrush.Dispose()
$font.Dispose()
$family.Dispose()

Write-Host "Generated placeholder icon at $outputPath"
