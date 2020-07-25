Write-Host "Exporting html package to ESP8266 sources"

[System.Text.RegularExpressions.RegexOptions] $options = [System.Text.RegularExpressions.RegexOptions]::Multiline -bor [System.Text.RegularExpressions.RegexOptions]::IgnoreCase

function Replace-Script([System.Text.RegularExpressions.Match] $Match) {
    $local:filename = $Match.Groups["File"]
    $local:content = Get-Content -LiteralPath ("./dist" + $local:filename) -Raw
    $local:content = [regex]::Replace($local:content, "//# sourceMappingURL=[^\n]+", "", $options)
    return "<script>" + $local:content + "</script>";
}

function Replace-Css([System.Text.RegularExpressions.Match] $Match) {
    $local:filename = $Match.Groups["File"]
    $local:content = (Get-Content -LiteralPath ("./dist" + $local:filename) -Raw)
    $local:content = [regex]::Replace($local:content, "\/\*# sourceMappingURL=.*?\*/", "", $options)
    return "<style>" + $local:content + "</style>";
}

function Export()
{
    $local:content = Get-Content -LiteralPath "./dist/index.html" -Raw;
    $local:content = [regex]::Replace($local:content, "<script src=`"(?<File>[^`"]+)`"></script>", { param($Match) Replace-Script($Match) }, $options)
    $local:content = [regex]::Replace($local:content, "<link rel=`"stylesheet`" href=`"(?<File>[^`"]+)`">", { param($Match) Replace-Css($Match) }, $options)
    
    $local:content = @"
#include "WebServer.h"

#include <sys/pgmspace.h>

static const char webpage_index_local[] PROGMEM = R"###(
$($local:content)
)###";

const __FlashStringHelper* WebServer::webpage_index_content = reinterpret_cast<const __FlashStringHelper*>(webpage_index_local);
"@

    $local:content | Out-File -LiteralPath "../black-betty/WebServer_index.cpp" -Encoding UTF8 -Force
}
Export
