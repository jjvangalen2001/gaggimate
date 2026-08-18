$ErrorActionPreference = "Stop"

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$imageName = "gaggimate-emulator"
$containerName = "gaggimate-emulator"

Write-Host "GaggiMate emulator bouwen..."
docker build --tag $imageName --file (Join-Path $PSScriptRoot "Dockerfile") $repoRoot
if ($LASTEXITCODE -ne 0) { throw "De emulatorbuild is mislukt." }

$existing = docker container ls --all --quiet --filter "name=^/$containerName$"
if ($existing) {
    docker container stop $containerName | Out-Null
    docker container rm $containerName | Out-Null
}

docker run --rm --detach --name $containerName --publish "127.0.0.1:6080:6080" $imageName | Out-Null
if ($LASTEXITCODE -ne 0) { throw "De emulator kon niet worden gestart." }

Start-Sleep -Seconds 2
Start-Process "http://127.0.0.1:6080/vnc.html?autoconnect=1&resize=scale"
Write-Host "Emulator gestart: http://127.0.0.1:6080/vnc.html?autoconnect=1&resize=scale"
Write-Host "Stoppen: docker container stop $containerName"
