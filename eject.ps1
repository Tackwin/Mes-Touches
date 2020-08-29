$processes = Get-Process | Select id

foreach ($p in $processes) {
	$i = $p.id
	& "Injector.exe" -p $i -e "C:\Users\Tackwin\Documents\Code\Mes_Touches\win_hook.dll"
}