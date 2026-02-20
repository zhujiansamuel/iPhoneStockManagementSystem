
Add-Type -AssemblyName System.Drawing

$inputPath = "$PSScriptRoot\ico.png"
$outputPath = "$PSScriptRoot\app.ico"

if (-not (Test-Path $inputPath)) {
    Write-Error "Input file not found: $inputPath"
    exit 1
}

$bmp = [System.Drawing.Bitmap]::FromFile($inputPath)
$iconStream = New-Object System.IO.FileStream($outputPath, [System.IO.FileMode]::Create)

# ICO Header
# Reserved (2 bytes), Type (2 bytes, 1=ICO), Count (2 bytes)
$header = [byte[]] @(0, 0, 1, 0, 1, 0)
$iconStream.Write($header, 0, $header.Length)

# Image Directory Entry
# Width (1), Height (1), ColorCount (1), Reserved (1), Planes (2), BitCount (2), BytesInRes (4), ImageOffset (4)
$width = if ($bmp.Width -ge 256) { 0 } else { [byte]$bmp.Width }
$height = if ($bmp.Height -ge 256) { 0 } else { [byte]$bmp.Height }

# Save BMP to memory to get size
$memStream = New-Object System.IO.MemoryStream
$bmp.Save($memStream, [System.Drawing.Imaging.ImageFormat]::Png)
$pngBytes = $memStream.ToArray()
$size = $pngBytes.Length

$entry = New-Object byte[] 16
$entry[0] = $width
$entry[1] = $height
$entry[2] = 0 # Color count
$entry[3] = 0 # Reserved
$entry[4] = 1 # Planes
$entry[5] = 0 # Planes
$entry[6] = 32 # Bit count
$entry[7] = 0 # Bit count

# BytesInRes (Little Endian)
$entry[8] = $size -band 0xFF
$entry[9] = ($size -shr 8) -band 0xFF
$entry[10] = ($size -shr 16) -band 0xFF
$entry[11] = ($size -shr 24) -band 0xFF

# ImageOffset (Little Endian)
# Header (6) + 1 Directory Entry (16) = 22
$offset = 22
$entry[12] = $offset -band 0xFF
$entry[13] = ($offset -shr 8) -band 0xFF
$entry[14] = ($offset -shr 16) -band 0xFF
$entry[15] = ($offset -shr 24) -band 0xFF

$iconStream.Write($entry, 0, $entry.Length)
$iconStream.Write($pngBytes, 0, $pngBytes.Length)

$iconStream.Close()
$memStream.Close()
$bmp.Dispose()

Write-Host "Successfully converted $inputPath to $outputPath"
