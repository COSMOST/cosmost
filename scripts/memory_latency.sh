adb root
adb shell am force-stop $1

# without cosmos
adb logcat -c
adb shell setprop wrap.$1 \"\"
adb shell monkey -p $1 -c android.intent.category.LAUNCHER 1
sleep 8
adb shell dumpsys meminfo | grep $1
adb shell am force-stop $1
adb logcat -d -b events | grep am_activity_launch_time | grep $1 | tail -n1

# with cosmos
adb logcat -c
adb shell setprop wrap.$1 LD_PRELOAD=/system/lib/liblogfileops.so
adb shell monkey -p $1 -c android.intent.category.LAUNCHER 1
sleep 5
adb shell dumpsys meminfo | grep $1
adb shell am force-stop $1
adb logcat -d -b events | grep am_activity_launch_time | grep $1 | tail -n1
