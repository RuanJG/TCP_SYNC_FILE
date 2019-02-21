package com.rompa.jk.tcpfilesyncer;

import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Binder;
import android.os.Environment;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.util.Log;

import java.io.File;
import java.io.FileOutputStream;

/**
 * Created by RSL-030 on 2018/11/8.
 */

public class SBSFileSyncService extends Service {

    private  volatile boolean mBinded = false;
    private  String Data=null;
    private String mIP;
    private int mPort;
    private String mID;
    private TcpFileSyncer mTcpSyncer;
    private String SBS_DATA_FILENAME =  "Sleeve_App_BLE_Data/SBS_MP_Data.txt";//“内部存储”目录下的路径
    private String SBS_DATA_Backup_FILENAME = "SBSFileSyncer/SBS_MP_Raw_Data.txt" ;// 备份目录

    class SyncServiceBinder extends Binder
    {
        public SBSFileSyncService getService(){
            return SBSFileSyncService.this;
        }
    }
    private  SyncServiceBinder mServerBinder = new SyncServiceBinder();

    public interface OnServiceStatusListener {
        public void Log(String msg);
        public void notify(String msg);
        public void updateSetting(String ip, int port, String id, String pwd);
    }
    public void setOnServiceStatusListener( OnServiceStatusListener listener) {
        mClientListener = listener;
    }
    OnServiceStatusListener mClientListener = null;


    @Override
    public void onCreate()
    {
        super.onCreate();
        Log.e("Ruan","SBSFileSyncService onCreate !");
        loadSetting();
        mTcpSyncer = new TcpFileSyncer(this.getApplicationContext(),mTcpHandler, SBS_DATA_FILENAME,mIP,mPort,mID);
    }

    @Override
    public IBinder onBind(Intent intent) {
        Log.e("Ruan","SBSFileSyncService onBind ");
        mTcpSyncer.Login();
        return mServerBinder;
    }

    @Override
    public boolean onUnbind(Intent intent) {
        Log.e("Ruan","SBSFileSyncService onunBind !");
        mTcpSyncer.Logout();
        return true;
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Log.e("Ruan","SBSFileSyncService onStartCommand !");

        if( intent == null ) {
            //to stop self
            return super.onStartCommand(intent,flags,startId);
        }

        String cmd = intent.getStringExtra("cmd");
        if( cmd.equals("start")){
            mTcpSyncer.Login();
        }else if ( cmd.equals("stop")){
            mTcpSyncer.Logout();
        }else if( cmd.equals("tcpsetting")) {
            mTcpSyncer.mIP = intent.getStringExtra("ip");
            mTcpSyncer.mPort = intent.getIntExtra("port", 55555);
            mTcpSyncer.mId = intent.getStringExtra("id");
            mTcpSyncer.Reconnect();
            saveTcpSetting(mTcpSyncer.mIP, mTcpSyncer.mPort, mTcpSyncer.mId);
            if( mClientListener != null){
                mClientListener.updateSetting(mIP,mPort,mID,getPasswardSetting());
            }
        }else if( cmd.equals("getSetting")){
            if( mClientListener != null){
                mClientListener.updateSetting(mIP,mPort,mID,getPasswardSetting());
            }
        }else if( cmd.equals("setpwd")){
            savePasswardSetting(intent.getStringExtra("pwd"));
            if( mClientListener != null){
                mClientListener.updateSetting(mIP,mPort,mID,getPasswardSetting());
            }
        }else if( cmd.equals("newdata")){
            Log.e("Ruan:","data:"+intent.getStringExtra("data"));
            String str = intent.getStringExtra("data");
            writeExternalFileBytes(SBS_DATA_Backup_FILENAME,str.getBytes());
            if( !mTcpSyncer.isTcpConnected() )
                mTcpSyncer.Login();
            if( mClientListener != null)
                mClientListener.Log("data: "+intent.getStringExtra("data")+"\n");
        }
        return super.onStartCommand(intent,flags,startId);
    }

    @Override
    public void onDestroy()
    {
        //seem never be here
        super.onDestroy();
    }


    //client will receive these msg
    private Handler mTcpHandler = new Handler(){
        public void handleMessage(Message msg) {
            if( mClientListener != null )
                mClientListener.Log(msg.getData().getString("log"));
        }
    };

    private void loadSetting()
    {
        SharedPreferences sharedPref=  this.getSharedPreferences("setting.properties", Context.MODE_PRIVATE);
        mIP = sharedPref.getString("ip","127.0.0.1");
        mPort = sharedPref.getInt ( "port" ,55555);
        mID = sharedPref.getString( "id" ,"5");
    }
    private void saveTcpSetting(String ip, int port, String id)
    {
        SharedPreferences sharedPref=  this.getSharedPreferences("setting.properties",Context.MODE_PRIVATE);
        SharedPreferences.Editor editor = sharedPref.edit();
        editor.putString("ip",ip);
        editor.putInt("port",port);
        editor.putString("id",id);
        editor.commit();
        mIP = ip;
        mPort = port;
        mID = id;
    }
    public void savePasswardSetting(String pwd)
    {
        SharedPreferences sharedPref=  this.getSharedPreferences("setting.properties",Context.MODE_PRIVATE);
        SharedPreferences.Editor editor = sharedPref.edit();
        editor.putString("pwd",pwd);
        editor.commit();
    }
    public String getPasswardSetting()
    {
        SharedPreferences sharedPref=  this.getSharedPreferences("setting.properties", Context.MODE_PRIVATE);
        return  sharedPref.getString("pwd","a123");
    }
    public String getIp(){
        return mIP;
    }
    public String getID(){
        return mID;
    }
    public int getPort(){
        return mPort;
    }

    private  boolean writeExternalFileBytes(String filename, byte[] bytes)
    {
        try
        {
            if (Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED))
            {
                String fpath = Environment.getExternalStorageDirectory().getPath()+ File.separator + filename;//"/Sleeve_App_BLE_Data" + "/" + "SBS_MP_Data" + ".txt";
                //fpath = context.getExternalCacheDir().getAbsolutePath() + File.separator + filename;

                //打开文件输入流
                FileOutputStream outputStream =  openFileOutput(fpath, Context.MODE_APPEND); //new FileOutputStream(fpath);
                //outputStream.getChannel().position(pos);
                outputStream.write(bytes);
                outputStream.flush();
                outputStream.close();
                return true;
            }
            Log.e("Ruan","Could not get External Storage");
            return false;
        }catch (Exception e) {
            Log.e("Ruan","error in read file");
            e.printStackTrace();
            return false;
        }
    }


}
