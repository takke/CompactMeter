REM
REM taskkill で既に起動している CompactMeter を削除する
REM
REM 起動しているプロセスがない場合もエラーとならないように本バッチファイルでラップする
REM

taskkill /IM CompactMeter.exe

exit /b 0
