param(
    [string]$BaseUrl = "http://127.0.0.1:8080/api/v1",
    [string]$AllowedOrigin = "http://localhost:5173"
)

$ErrorActionPreference = "Stop"

$script:Passed = 0
$script:Failed = 0
$script:TempHuespedId = $null
$script:TempHabitacionId = $null
$script:TempRes1Id = $null
$script:TempRes2Id = $null

function Assert-Check {
    param(
        [string]$Name,
        [bool]$Condition,
        [string]$Detail = ""
    )
    if ($Condition) {
        $script:Passed++
        Write-Host "[PASS] $Name" -ForegroundColor Green
    } else {
        $script:Failed++
        Write-Host "[FAIL] $Name :: $Detail" -ForegroundColor Red
    }
}

function Invoke-ApiRequest {
    param(
        [string]$Method,
        [string]$Path,
        [string]$Body = "",
        [string]$ContentType = "application/json",
        [hashtable]$Headers = @{}
    )
    $uri = "$BaseUrl$Path"
    $params = @{
        Method          = $Method
        Uri             = $uri
        UseBasicParsing = $true
        Headers         = $Headers
        TimeoutSec      = 15
    }
    if ($Body.Length -gt 0) {
        $params["Body"] = $Body
        if ($ContentType.Length -gt 0) {
            $params["ContentType"] = $ContentType
        }
    }

    try {
        $resp = Invoke-WebRequest @params
        $json = $null
        if ($resp.Content -and $resp.Content.Trim().Length -gt 0) {
            try { $json = $resp.Content | ConvertFrom-Json } catch {}
        }
        return [PSCustomObject]@{
            Status  = [int]$resp.StatusCode
            Json    = $json
            Headers = $resp.Headers
            Raw     = $resp.Content
        }
    } catch {
        $ex = $_.Exception
        $status = -1
        $rawBody = ""
        $json = $null

        if ($null -ne $ex.Response) {
            $httpResp = $ex.Response -as [System.Net.HttpWebResponse]
            if ($null -ne $httpResp) {
                $status = [int]$httpResp.StatusCode
            } else {
                try { $status = [int]$ex.Response.StatusCode.value__ } catch {}
                if ($status -le 0) {
                    try { $status = [int]$ex.Response.StatusCode } catch {}
                }
            }
            try {
                $stream = $ex.Response.GetResponseStream()
                if ($null -ne $stream -and $stream.CanRead) {
                    $reader = New-Object System.IO.StreamReader($stream)
                    $rawBody = $reader.ReadToEnd()
                    $reader.Close()
                    if ($rawBody.Trim().Length -gt 0) {
                        try { $json = $rawBody | ConvertFrom-Json } catch {}
                    }
                }
            } catch {}
        }

        if ($status -le 0 -and $ex.Message -match "\((\d{3})\)") {
            $status = [int]$Matches[1]
        }

        return [PSCustomObject]@{
            Status  = $status
            Json    = $json
            Headers = $null
            Raw     = if ($rawBody) { $rawBody } else { $ex.Message }
        }
    }
}

$tag = [DateTimeOffset]::UtcNow.ToUnixTimeMilliseconds().ToString()
$shortTag = $tag.Substring([Math]::Max(0, $tag.Length - 5))

Write-Host "=== Suite de Pruebas HTTP API REST ($BaseUrl) ===" -ForegroundColor Cyan

try {
    # 1. Salud del sistema
    $r = Invoke-ApiRequest -Method "Get" -Path "/health"
    Assert-Check "01. GET /health responde 200 y status=ok" ($r.Status -eq 200 -and $r.Json.status -eq "ok") "HTTP=$($r.Status) Raw=$($r.Raw)"

    # 2. Seguridad: CORS permitido, CORS denegado, Content-Type invalido y bloqueo NoSQLi
    $r = Invoke-ApiRequest -Method "Options" -Path "/huespedes" -Headers @{ Origin = $AllowedOrigin }
    Assert-Check "02. OPTIONS /huespedes (CORS permitido) -> 204" ($r.Status -eq 204 -and $r.Headers["Access-Control-Allow-Origin"] -eq $AllowedOrigin) "HTTP=$($r.Status)"

    $r = Invoke-ApiRequest -Method "Get" -Path "/huespedes" -Headers @{ Origin = "http://origen-no-autorizado.test" }
    Assert-Check "03. GET /huespedes (CORS no autorizado) -> 403" ($r.Status -eq 403) "HTTP=$($r.Status) Raw=$($r.Raw)"

    $r = Invoke-ApiRequest -Method "Post" -Path "/huespedes" -ContentType "text/plain" -Body "texto plano"
    Assert-Check "04. POST /huespedes con Content-Type text/plain -> 415" ($r.Status -eq 415) "HTTP=$($r.Status) Raw=$($r.Raw)"

    $noSqlPayload = '{"nombre":"Atk","correo":{"$ne":""},"telefono":"4770000001"}'
    $r = Invoke-ApiRequest -Method "Post" -Path "/huespedes" -Body $noSqlPayload
    Assert-Check "05. POST /huespedes con operador NoSQL (`$ne) -> 400" ($r.Status -eq 400) "HTTP=$($r.Status) Raw=$($r.Raw)"

    # 3. CRUD Huespedes + casos borde
    $hBody = @{
        nombre       = "Huesped API $shortTag"
        correo       = "api.test.$shortTag@ejemplo.com"
        telefono     = "4771002003"
        preferencias = @("piso alto", "no fumar")
    } | ConvertTo-Json

    $r = Invoke-ApiRequest -Method "Post" -Path "/huespedes" -Body $hBody
    Assert-Check "06. POST /huespedes crea registro -> 201" ($r.Status -eq 201 -and $null -ne $r.Json.id) "HTTP=$($r.Status) Raw=$($r.Raw)"
    $script:TempHuespedId = $r.Json.id

    $r = Invoke-ApiRequest -Method "Post" -Path "/huespedes" -Body $hBody
    Assert-Check "07. POST /huespedes correo duplicado -> 409" ($r.Status -eq 409) "HTTP=$($r.Status) Raw=$($r.Raw)"

    $r = Invoke-ApiRequest -Method "Get" -Path "/huespedes?correo=inexistente.$shortTag@ejemplo.com"
    Assert-Check "08. GET /huespedes sin coincidencias -> 200 con []" ($r.Status -eq 200 -and $r.Json.meta.total -eq 0) "HTTP=$($r.Status) Raw=$($r.Raw)"

    $r = Invoke-ApiRequest -Method "Get" -Path "/huespedes/id-mal-formado"
    Assert-Check "09. GET /huespedes/{id} invalido -> 400" ($r.Status -eq 400) "HTTP=$($r.Status) Raw=$($r.Raw)"

    $r = Invoke-ApiRequest -Method "Get" -Path "/huespedes/65a000000000000000000099"
    Assert-Check "10. GET /huespedes/{id} inexistente -> 404" ($r.Status -eq 404) "HTTP=$($r.Status) Raw=$($r.Raw)"

    $r = Invoke-ApiRequest -Method "Patch" -Path "/huespedes/$($script:TempHuespedId)" -Body (@{ telefono = "4779991122" } | ConvertTo-Json)
    Assert-Check "11. PATCH /huespedes/{id} actualiza telefono -> 200" ($r.Status -eq 200 -and $r.Json.telefono -eq "4779991122") "HTTP=$($r.Status) Raw=$($r.Raw)"

    # 4. CRUD Habitaciones + cambio de estado
    $habBody = @{
        numero    = "AP-$shortTag"
        tipo      = "suite"
        capacidad = 4
        tarifa    = 2000.0
        servicios = @("wifi", "balcon")
    } | ConvertTo-Json

    $r = Invoke-ApiRequest -Method "Post" -Path "/habitaciones" -Body $habBody
    Assert-Check "12. POST /habitaciones crea habitacion -> 201" ($r.Status -eq 201 -and $null -ne $r.Json.id) "HTTP=$($r.Status) Raw=$($r.Raw)"
    $script:TempHabitacionId = $r.Json.id

    $r = Invoke-ApiRequest -Method "Patch" -Path "/habitaciones/$($script:TempHabitacionId)/estado" -Body (@{ estado = "mantenimiento" } | ConvertTo-Json)
    Assert-Check "13. PATCH /habitaciones/{id}/estado -> mantenimiento (200)" ($r.Status -eq 200 -and $r.Json.estado -eq "mantenimiento") "HTTP=$($r.Status) Raw=$($r.Raw)"

    $r = Invoke-ApiRequest -Method "Patch" -Path "/habitaciones/$($script:TempHabitacionId)/estado" -Body (@{ estado = "disponible" } | ConvertTo-Json)
    Assert-Check "14. PATCH /habitaciones/{id}/estado -> disponible (200)" ($r.Status -eq 200 -and $r.Json.estado -eq "disponible") "HTTP=$($r.Status) Raw=$($r.Raw)"

    # 5. Reservaciones, traslapes, borde exacto y disponibilidad
    $res1Body = @{
        huespedId    = $script:TempHuespedId
        habitacionId = $script:TempHabitacionId
        fechaEntrada = "2028-08-10"
        fechaSalida  = "2028-08-13"
        personas     = 2
    } | ConvertTo-Json

    $r = Invoke-ApiRequest -Method "Post" -Path "/reservaciones" -Body $res1Body
    Assert-Check "15. POST /reservaciones calcula noches=3 y total=6000 -> 201" ($r.Status -eq 201 -and $r.Json.noches -eq 3 -and [double]$r.Json.total -eq 6000.0) "HTTP=$($r.Status) Raw=$($r.Raw)"
    $script:TempRes1Id = $r.Json.id

    $r = Invoke-ApiRequest -Method "Post" -Path "/reservaciones" -Body $res1Body
    Assert-Check "16. POST /reservaciones traslapada rechazada -> 409" ($r.Status -eq 409) "HTTP=$($r.Status) Raw=$($r.Raw)"

    $r = Invoke-ApiRequest -Method "Get" -Path "/disponibilidad?fechaEntrada=2028-08-11&fechaSalida=2028-08-12"
    $excluded = ($r.Status -eq 200) -and -not ($r.Json.habitaciones | Where-Object { $_.id -eq $script:TempHabitacionId })
    Assert-Check "17. GET /disponibilidad excluye habitacion reservada" $excluded "HTTP=$($r.Status) Raw=$($r.Raw)"

    $res2Body = @{
        huespedId    = $script:TempHuespedId
        habitacionId = $script:TempHabitacionId
        fechaEntrada = "2028-08-13"
        fechaSalida  = "2028-08-15"
        personas     = 2
    } | ConvertTo-Json
    $r = Invoke-ApiRequest -Method "Post" -Path "/reservaciones" -Body $res2Body
    Assert-Check "18. POST /reservaciones en borde check-out=check-in -> 201" ($r.Status -eq 201 -and [double]$r.Json.total -eq 4000.0) "HTTP=$($r.Status) Raw=$($r.Raw)"
    $script:TempRes2Id = $r.Json.id

    # 6. Reportes y validacion de rango maximo (> 366 dias -> 422)
    $r = Invoke-ApiRequest -Method "Get" -Path "/reportes/resumen?fechaInicio=2028-08-01&fechaFin=2028-08-31"
    Assert-Check "19. GET /reportes/resumen suma ingresos del periodo -> 200" ($r.Status -eq 200 -and [double]$r.Json.ingresosEstimados -ge 10000.0) "HTTP=$($r.Status) Raw=$($r.Raw)"

    $r = Invoke-ApiRequest -Method "Get" -Path "/reportes/resumen?fechaInicio=2026-01-01&fechaFin=2028-01-01"
    Assert-Check "20. GET /reportes/resumen rango >366 dias -> 422" ($r.Status -eq 422) "HTTP=$($r.Status) Raw=$($r.Raw)"

    # 7. Cancelacion + baja logica + eliminacion fisica controlada (delete_one)
    $r = Invoke-ApiRequest -Method "Post" -Path "/reservaciones/$($script:TempRes1Id)/cancelar"
    Assert-Check "21. POST /reservaciones/{id}/cancelar -> 200 (cancelada)" ($r.Status -eq 200 -and $r.Json.estado -eq "cancelada") "HTTP=$($r.Status) Raw=$($r.Raw)"

    $null = Invoke-ApiRequest -Method "Post" -Path "/reservaciones/$($script:TempRes2Id)/cancelar"
    $d1 = Invoke-ApiRequest -Method "Delete" -Path "/reservaciones/$($script:TempRes1Id)"
    $d2 = Invoke-ApiRequest -Method "Delete" -Path "/reservaciones/$($script:TempRes2Id)"
    $script:TempRes1Id = $null
    $script:TempRes2Id = $null

    $dHab = Invoke-ApiRequest -Method "Delete" -Path "/habitaciones/$($script:TempHabitacionId)"
    $script:TempHabitacionId = $null

    $softH = Invoke-ApiRequest -Method "Delete" -Path "/huespedes/$($script:TempHuespedId)"
    $hardH = Invoke-ApiRequest -Method "Delete" -Path "/huespedes/$($script:TempHuespedId)?fisico=true"
    $script:TempHuespedId = $null

    $cleanupOk = ($d1.Status -eq 204 -and $d2.Status -eq 204 -and $dHab.Status -eq 204 -and $softH.Status -eq 200 -and $hardH.Status -eq 204)
    Assert-Check "22. Baja logica + eliminaciones fisicas controladas (200/204)" $cleanupOk "d1=$($d1.Status) d2=$($d2.Status) dHab=$($dHab.Status) softH=$($softH.Status) hardH=$($hardH.Status)"
}
finally {
    foreach ($resId in @($script:TempRes1Id, $script:TempRes2Id)) {
        if ($resId) {
            $null = Invoke-ApiRequest -Method "Post" -Path "/reservaciones/$resId/cancelar"
            $null = Invoke-ApiRequest -Method "Delete" -Path "/reservaciones/$resId"
        }
    }
    if ($script:TempHabitacionId) {
        $null = Invoke-ApiRequest -Method "Delete" -Path "/habitaciones/$($script:TempHabitacionId)"
    }
    if ($script:TempHuespedId) {
        $null = Invoke-ApiRequest -Method "Delete" -Path "/huespedes/$($script:TempHuespedId)?fisico=true"
    }
}

Write-Host ""
Write-Host "Resumen API Tests -> Passed: $($script:Passed) | Failed: $($script:Failed)" -ForegroundColor Cyan
if ($script:Failed -gt 0) {
    exit 1
}
exit 0