package com.rompa.jk.tcpfilesyncer;

import android.app.Activity;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.os.Binder;
import android.os.Environment;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.Serializable;
import java.util.Properties;


public class MainActivity extends Activity {

    EditText ipText ;
    EditText idText ;
    EditText portText;
    private  EditText console;
    private EditText pwdTextEdit;
    private Button setPwdButton;
    private Button setSettingButton;
    private String SBS_DATA_FILENAME =  "Sleeve_App_BLE_Data/SBS_MP_Data.txt";//“内部存储”目录下的路径
    private TcpFileSyncer mTcpSyncer;
    Properties properties;
    private Handler mServiceHandler;
    Context mContext;
    SBSFileSyncService mServer ;
    String mPassword = "a123";

    private Handler mHandler = new Handler(){
        public void handleMessage(Message msg) {
            if( console == null) {
                console = (EditText) findViewById(R.id.console);
            }
            console.append(msg.getData().getString("log"));

            String notify = msg.getData().getString("notify");
            if( notify.length() > 0 ){
                notifyUser("Sync File:", notify);
            }
        }
    };


    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        console = (EditText) this.findViewById(R.id.console);
        ipText = (EditText) this.findViewById(R.id.ipEditText);
        idText = (EditText) this.findViewById(R.id.idEditText);
        portText = (EditText) this.findViewById(R.id.portEditText);
        pwdTextEdit = (EditText) this.findViewById(R.id.pwdEditText);
        setPwdButton = (Button) this.findViewById(R.id.setPwdBuuton);
        setSettingButton = (Button) this.findViewById(R.id.setSettingbutton);
        setSettingButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Intent mIintent = new Intent();
                mIintent.putExtra("cmd","tcpsetting");
                mIintent.putExtra("ip",ipText.getText().toString());
                mIintent.putExtra("port", Integer.parseInt(portText.getText().toString()));
                mIintent.putExtra("id",idText.getText().toString());
                mIintent.setComponent(new ComponentName("com.rompa.jk.tcpfilesyncer","com.rompa.jk.tcpfilesyncer.SBSFileSyncService"));
                mContext.startService(mIintent);
            }
        });
        setPwdButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if( !mPwdEditStatus ){
                    mPwdEditStatus = true;
                    setPwdButton.setText("OK");
                    return;
                }


                Intent mIintent = new Intent();
                mIintent.putExtra("cmd","setpwd");
                mIintent.putExtra("pwd",pwdTextEdit.getText().toString());
                mIintent.setComponent(new ComponentName("com.rompa.jk.tcpfilesyncer","com.rompa.jk.tcpfilesyncer.SBSFileSyncService"));
                mContext.startService(mIintent);

                setPwdButton.setText("Modify");
                mPwdEditStatus = false;
            }
        });
        setUnloginedStatus();
        pwdTextEdit.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence charSequence, int i, int i1, int i2) {

            }
            @Override
            public void onTextChanged(CharSequence charSequence, int i, int i1, int i2) {
                if( mPwdEditStatus ) return;
                if( mPassword.equals( charSequence.toString())) {
                    setLoginedStatus();
                }else{
                    setUnloginedStatus();
                }
            }
            @Override
            public void afterTextChanged(Editable editable) {

            }
        });

        mContext  = this.getApplicationContext();
        bindFileSyncService();
    }

    @Override
    protected void onResume() {
        super.onResume();
        Log.e("Ruan","onResume");

        if( mContext == null ) return; // the bind and init will be done in onCreat

        //mContext  = this.getApplicationContext();
        bindFileSyncService();
        Intent mIintent = new Intent();
        mIintent.putExtra("cmd","start");
        mIintent.setComponent(new ComponentName("com.rompa.jk.tcpfilesyncer","com.rompa.jk.tcpfilesyncer.SBSFileSyncService"));
        mContext.startService(mIintent);
    }

    @Override
    protected void onDestroy() {
        Log.e("Ruan","onDestroy");
        mContext  = this.getApplicationContext();
        unBindFileSyncService();
        super.onDestroy();
    }

    private volatile boolean hasbinded = false;
    ServiceConnection serviceConnection;
    private boolean bindFileSyncService()
    {
        if( hasbinded ) return true;
        serviceConnection = new ServiceConnection() {
            @Override
            public void onServiceConnected(ComponentName name, IBinder service) {
                SBSFileSyncService.SyncServiceBinder binder = (SBSFileSyncService.SyncServiceBinder) service;
                mServer = binder.getService();
                mServer.setOnServiceStatusListener(new SBSFileSyncService.OnServiceStatusListener() {
                    @Override
                    public void Log(String msg) {
                        console.append(msg);
                    }
                    @Override
                    public void notify(String msg) {
                        console.append(msg);
                    }
                    @Override
                    public void updateSetting(String ip, int port, String id, String pwd){
                        ipText.setText(ip);
                        portText.setText( String.valueOf(port));
                        idText.setText(id);
                        mPassword = pwd;
                    }
                });
                Intent mIintent = new Intent();
                mIintent.putExtra("cmd","getSetting");
                mIintent.setComponent(new ComponentName("com.rompa.jk.tcpfilesyncer","com.rompa.jk.tcpfilesyncer.SBSFileSyncService"));
                mContext.startService(mIintent);
//            ipText.setText(mServer.getIp());
//            portText.setText( String.valueOf( mServer.getPort() ));
//            idText.setText(mServer.getID());
            }
            @Override
            public void onServiceDisconnected(ComponentName name) {
            }
        };

        Intent intent =  new Intent(MainActivity.this, SBSFileSyncService.class);
        hasbinded = mContext.bindService(intent, serviceConnection, BIND_AUTO_CREATE);
        return hasbinded;
    }
    private void unBindFileSyncService()
    {
        if( hasbinded) {
            hasbinded = false;
            mServer.setOnServiceStatusListener(null);
            mContext.unbindService(serviceConnection);
        }
    }

    private volatile boolean mUserLogined = true;
    private volatile boolean mPwdEditStatus = false;
    private void setUnloginedStatus()
    {
        if( !mUserLogined ) return ;

        ipText.setEnabled(false);
        idText.setEnabled(false);
        portText.setEnabled(false);
        pwdTextEdit.setEnabled(true);
        setPwdButton.setEnabled(false);
        setSettingButton.setEnabled(false);
        console.setEnabled(false);

        mUserLogined = false;
    }

    private void setLoginedStatus()
    {
        if( mUserLogined ) return ;

        ipText.setEnabled(true);
        idText.setEnabled(true);
        portText.setEnabled(true);
        pwdTextEdit.setEnabled(true);
        setPwdButton.setEnabled(true);
        setSettingButton.setEnabled(true);
        setPwdButton.setText("Modify");

        mUserLogined = true;
    }

    private void notifyUser(String title ,String msg)
    {
        NotificationManager manager = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
        //创建通知建设类
        Notification.Builder builder = new Notification.Builder(MainActivity.this);
        //设置跳转的页面
        PendingIntent intent = PendingIntent.getActivity(MainActivity.this,
                100, new Intent(MainActivity.this, MainActivity.class),
                PendingIntent.FLAG_CANCEL_CURRENT);

        //设置通知栏标题
        builder.setContentTitle(title);
        //设置通知栏内容
        builder.setContentText(msg);
        //设置跳转
        builder.setContentIntent(intent);
        //设置图标
        //builder.setSmallIcon(R.mipmap.ic_launcher);
        //设置
        builder.setDefaults(Notification.DEFAULT_ALL);
        //创建通知类
        Notification notification = builder.build();
        //显示在通知栏
        manager.notify(0, notification);
    }




}





