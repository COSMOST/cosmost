#adb reboot
#sleep 60
#adb root
#sleep 10

if [ ! -d $1 ]; then
	mkdir $1
fi

# without
adb shell am startservice --user 0 com.quicinc.trepn/.TrepnService &>> $1/default.log
adb logcat -c
echo "Starting profiler"
adb shell am broadcast -a com.quicinc.trepn.start_profiling -e com.quicinc.trepn.database_file "default.db" &>> $1/default.log
adb shell date > $1/default.launch_latency
sleep 2
echo "Starting app $1"
adb shell monkey -p $1 -c android.intent.category.LAUNCHER 1 &>> $1/default.log
sleep 180
echo "Stopping profiler"
adb shell am broadcast -a com.quicinc.trepn.stop_profiling &>> $1/default.log
echo "Stopping app $1"
adb shell am force-stop $1
sleep 20
adb logcat -d -b events | grep am_activity_launch_time | grep $1 | tail -n1 >> $1/default.launch_latency 
adb shell am broadcast -a com.quicinc.trepn.export_to_csv -e com.quicinc.trepn.export_db_input_file "default.db" -e com.quicinc.trepn.export_csv_output_file  "default.csv" &>> $1/default.log
sleep 20
echo "Pulling results"
adb pull /sdcard/trepn/default.db &>> $1/default.log
adb pull /sdcard/trepn/default.csv &>> $1/default.log
adb shell rm /sdcard/trepn/default.db
adb shell rm /sdcard/trepn/default.csv

# without cosmos
adb shell setprop wrap.$1 LD_PRELOAD=/system/lib/liblogfileops.so
adb shell am startservice --user 0 com.quicinc.trepn/.TrepnService &>> $1/cosmos.log
adb logcat -c
echo "Starting profiler"
adb shell am broadcast -a com.quicinc.trepn.start_profiling -e com.quicinc.trepn.database_file "cosmos.db" &>> $1/cosmos.log
sleep 2
echo "Starting app $1"
adb shell date > $1/cosmos.launch_latency
adb shell monkey -p $1 -c android.intent.category.LAUNCHER 1 &>> $1/cosmos.log
sleep 180
echo "Stopping profiler"
adb shell am broadcast -a com.quicinc.trepn.stop_profiling &>> $1/cosmos.log
echo "Stopping app $1"
adb shell am force-stop $1
sleep 20
adb logcat -d -b events | grep am_activity_launch_time | grep $1 | tail -n1 >> $1/cosmos.launch_latency 
adb shell am broadcast -a com.quicinc.trepn.export_to_csv -e com.quicinc.trepn.export_db_input_file "cosmos.db" -e com.quicinc.trepn.export_csv_output_file  "cosmos.csv" &>> $1/cosmos.log
sleep 20
echo "Pulling results"
adb pull /sdcard/trepn/cosmos.db &>> $1/cosmos.log
adb pull /sdcard/trepn/cosmos.csv &>> $1/cosmos.log
adb shell rm /sdcard/trepn/cosmos.db
adb shell rm /sdcard/trepn/cosmos.csv

mv cosmos.db $1/.
mv cosmos.csv $1/.
mv default.db $1/.
mv default.csv $1/.
