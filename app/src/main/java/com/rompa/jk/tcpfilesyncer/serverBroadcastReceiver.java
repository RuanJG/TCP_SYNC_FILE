package com.rompa.jk.tcpfilesyncer;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.Message;
import android.util.Log;

/**
 * Created by RSL-030 on 2018/11/9.
 */

public class serverBroadcastReceiver extends BroadcastReceiver {
    @Override
    public void onReceive(Context context, Intent intent) {
        String message;
        Bundle bundle = intent.getBundleExtra("tcpBundle");
        message = bundle.getString("log");
        Log.e("Ruan","log:"+message);
        message = bundle.getString("notify");
        Log.e("Ruan","notify:"+message);
    }

    public void userhandleMessage(Message msg) {
        Intent intent = new Intent();
        intent.setAction("com.rompa.jk.tcpfilesyncer.broadcastereceiver");
        intent.putExtra("tcpBundle",msg.getData());
        //发送广播--普通
        //sendBroadcast(intent);
        //发送有序广播
        //sendOrderedBroadcast(intent,null);
    }
}
