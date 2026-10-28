param(
    [string]$FirmwarePackage = "$env:USERPROFILE\STM32Cube\Repository\STM32Cube_FW_F3_V1.11.6"
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$outputDir = Join-Path $projectRoot 'MDK-ARM\gcc'
$null = New-Item -ItemType Directory -Force -Path $outputDir
$compiler = (Get-Command arm-none-eabi-gcc.exe -ErrorAction Stop).Source
$binDir = Split-Path $compiler -Parent
$drivers = Join-Path $FirmwarePackage 'Drivers'
$startup = Join-Path $drivers 'CMSIS\Device\ST\STM32F3xx\Source\Templates\gcc\startup_stm32f334x8.s'
$linker = Join-Path $FirmwarePackage 'Projects\STM32F3348-Discovery\Templates\SW4STM32\STM32F3348-Discovery\STM32F334C8Tx_FLASH.ld'
if (!(Test-Path -LiteralPath $startup) -or !(Test-Path -LiteralPath $linker)) {
    throw "STM32Cube F3 package is incomplete: $FirmwarePackage"
}
$flags = @('-mcpu=cortex-m4', '-mthumb', '-mfpu=fpv4-sp-d16', '-mfloat-abi=hard',
    '-Os', '-g3', '-ffunction-sections', '-fdata-sections', '-Wall', '-std=gnu11',
    '-DUSE_HAL_DRIVER', '-DSTM32F334x8')
$includes = @("$projectRoot\Core\Inc", "$projectRoot\Core\MyCore",
    "$drivers\STM32F3xx_HAL_Driver\Inc", "$drivers\STM32F3xx_HAL_Driver\Inc\Legacy",
    "$drivers\CMSIS\Device\ST\STM32F3xx\Include", "$drivers\CMSIS\Include")
foreach ($include in $includes) { $flags += "-I$include" }
$sources = @(Get-ChildItem "$projectRoot\Core\Src\*.c", "$projectRoot\Core\MyCore\*.c" | Sort-Object FullName | ForEach-Object FullName)
$halModules = @('hal', 'hal_adc', 'hal_adc_ex', 'hal_can', 'hal_cortex', 'hal_dac',
    'hal_dac_ex', 'hal_dma', 'hal_exti', 'hal_flash', 'hal_flash_ex', 'hal_gpio',
    'hal_hrtim', 'hal_i2c', 'hal_i2c_ex', 'hal_pwr', 'hal_pwr_ex', 'hal_rcc',
    'hal_rcc_ex', 'hal_tim', 'hal_tim_ex', 'hal_uart', 'hal_uart_ex')
foreach ($module in $halModules) { $sources += "$drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_$module.c" }
$sources += $startup
$objects = @()
foreach ($source in $sources) {
    $object = Join-Path $outputDir (([IO.Path]::GetFileName($source)) + '.o')
    Write-Host "Compiling $([IO.Path]::GetFileName($source))"
    & $compiler @flags -c $source -o $object
    if ($LASTEXITCODE -ne 0) { throw "Compilation failed: $source" }
    $objects += $object
}
$elf = Join-Path $outputDir 'hysteresis_cap_f334.elf'
$map = Join-Path $outputDir 'hysteresis_cap_f334.map'
& $compiler @flags @objects "-T$linker" '--specs=nano.specs' '--specs=nosys.specs' '-Wl,--gc-sections' "-Wl,-Map=$map" '-Wl,--print-memory-usage' '-Wl,--start-group' '-lc' '-lm' '-Wl,--end-group' -o $elf
if ($LASTEXITCODE -ne 0) { throw 'Link failed' }
foreach ($format in @('ihex', 'binary')) {
    $extension = if ($format -eq 'ihex') { 'hex' } else { 'bin' }
    & "$binDir\arm-none-eabi-objcopy.exe" -O $format $elf (Join-Path $outputDir "hysteresis_cap_f334.$extension")
    if ($LASTEXITCODE -ne 0) { throw "Failed to generate $extension" }
}
& "$binDir\arm-none-eabi-size.exe" $elf
if ($LASTEXITCODE -ne 0) { throw 'Size inspection failed' }
Write-Host "Build succeeded: $elf"
