package com.rompa.jk.tcpfilesyncer;

import android.Manifest;
import android.app.AlertDialog;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AppCompatActivity;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.TextView;

import com.uuzuche.lib_zxing.activity.CaptureActivity;
import com.uuzuche.lib_zxing.activity.CaptureFragment;
import com.uuzuche.lib_zxing.activity.CodeUtils;
import com.uuzuche.lib_zxing.activity.ZXingLibrary;

import java.io.File;
import java.io.FileOutputStream;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Properties;


public class MainActivity extends AppCompatActivity {

    private  EditText console;
    private Button setSettingButton;
    private Button exportButton;
    private Button scanButton;
    private EditText POEditText;
    private EditText BoxEditText;
    private TextView BoxCounter;
    private int Boxcount;
    private String mDataDir =  "Project_18-117_出货记录";//“内部存储”目录下的路径
    private String mDataFileName =  "BoxBarcodeList";//“内部存储”目录下的路径
    private TcpFileSyncer mTcpSyncer;
    Properties properties;
    private Handler mServiceHandler;
    Context mContext;
    //SBSFileSyncService mServer ;
    private DBHelper mDatabase;
    private boolean startedRecord = false;
    private int recordSeq = 1;

    private final int SCAN_REQUEST_CODE = 0xff;

    private boolean startCamera = false;
    CaptureFragment captureFragment;
    private final String GuideString="操作指引：\n1，  输入订单号\n2，  点击<开始录入>按键\n3，   输入包装箱序列号\n4，   录入完成后，点击<完成>按键";

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.my_scaner_layout);

        POEditText = (EditText) this.findViewById(R.id.idEditText);
        BoxEditText = (EditText) this.findViewById(R.id.boxeditText);
        console = (EditText) this.findViewById(R.id.console);
        setSettingButton = (Button) this.findViewById(R.id.setSettingbutton);
        BoxCounter = (TextView) this.findViewById(R.id.BoxCounttextView3);
        exportButton = (Button) this.findViewById(R.id.exportButton);
        scanButton = (Button) this.findViewById(R.id.scanbutton2);

        console.setEnabled(false);
        recordSeq = 1;
        Boxcount = 0;


        startedRecord = false;
        POEditText.setEnabled(true);
        BoxEditText.setEnabled(false);
        scanButton.setEnabled(false);
        POEditText.requestFocus();
        setSettingButton.setText("开始录入");
        console.append(GuideString);

        setSettingButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if( startedRecord ){
                    console.setText(GuideString);
                    startedRecord = false;
                    POEditText.setEnabled(true);
                    BoxEditText.setEnabled(false);
                    scanButton.setEnabled(false);
                    POEditText.requestFocus();
                    setSettingButton.setText("开始录入");
                }else{
                    startedRecord = true;
                    POEditText.setEnabled(false);
                    BoxEditText.setEnabled(true);
                    scanButton.setEnabled(true);
                    BoxEditText.requestFocus();
                    setSettingButton.setText("完成");
                    recordSeq = 1;
                    Boxcount = 0;
                    String[] boxlist = mDatabase.getPOBoxList(POEditText.getText().toString());
                    console.setText("开始记录订单（"+POEditText.getText().toString()+"）的箱号：\n\n");
                    console.append("序号，  包装箱序列号，    录入日期 \n");
                    for( int i=0; i< boxlist.length; i++){
                        console.append((recordSeq++)+","+boxlist[i]+"\n");
                        Boxcount++;
                    }
                    BoxCounter.setText(Boxcount+"");
                }
            }
        });

        exportButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if( mDatabase == null ) {
                    showMessageBox("数据库打开失败，无法导出数据!!!");
                    return;
                }
                Date ddate = new Date(System.currentTimeMillis());
                SimpleDateFormat smt = new SimpleDateFormat("yyyyMMdd-HHmmss");
                String fileName =  getExStorageFilePath(mDataDir,mDataFileName+"_"+smt.format(ddate)+".txt");
                if ( fileName.length() > 0) {
                    if( writeExternalFileBytes(fileName, mDatabase.getAllRecord())){
                        showMessageBox("导出数据在"+fileName);
                    }else{
                        showMessageBox("导出数据失败!!!");
                    }
                }else{
                    showMessageBox("手机存储区异常，打开txt文件失败!!!");
                }
            }
        });

        BoxEditText.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {
                //Log.d("Ruan", "beforeTextChanged: s = " + s + ", start = " + start +", count = " + count + ", after = " + after);
            }
            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {
                //Log.d("Ruan", "onTextChanged: s = " + s + ", start = " + start +", before = " + before + ", count = " + count);
                final String boxcode = s.toString();
                if( boxcode.length() >= 20){
                    Date date = new Date(System.currentTimeMillis());
                    SimpleDateFormat simpleDateFormat = new SimpleDateFormat("yyyyMMdd");//new SimpleDateFormat("yyyyMMdd-HH:mm:ss");

                    if( mDatabase == null ) return ;
                    String exitedPO = mDatabase.queryPObyBoxCode(boxcode);
                    //console.append("exitPO . len="+exitedPO+" , "+exitedPO.length()+"\n");
                    if( exitedPO.length() > 0){
                        AlertDialog.Builder builder =  new AlertDialog.Builder(MainActivity.this);
                        final String[] selection = new String[]{"删除","更新"};
                        builder.setTitle("这个箱已在订单"+POEditText.getText().toString()+"里，选择操作：");
                        //builder.setIcon(null);
                        builder.setItems(selection, new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                // TODO Auto-generated method stub
                                //console.append("choise "+selection[which]+"\n");
                                if(which == 0){
                                    //delete
                                    String currentRecord = mDatabase.queryPObyBoxCode(boxcode);
                                    if( mDatabase.deleteBox(boxcode) ){
                                        console.append(selection[which]+": \n"+currentRecord);
                                        Boxcount --;
                                        BoxCounter.setText(Boxcount+"");
                                    }else{
                                        console.append(selection[which]+": \n"+currentRecord +"失败");
                                    }
                                }else{
                                    //update
                                    Date ddate = new Date(System.currentTimeMillis());
                                    SimpleDateFormat smt = new SimpleDateFormat("yyyyMMdd");//new SimpleDateFormat("yyyyMMdd-HH:mm:ss");
                                    if( mDatabase.replaceBox(POEditText.getText().toString(), boxcode,recordSeq,smt.format(ddate))){
                                        String cr = mDatabase.queryPObyBoxCode(boxcode);
                                        console.append(selection[which]+": \n"+recordSeq+",   "+cr);
                                        recordSeq++;
                                    }else{
                                        console.append(selection[which]+": "+"失败");
                                    }
                                }
                                dialog.dismiss();
                            }
                        });
                        builder.setNegativeButton("取消",null);
                        builder.show();
                    }else{
                        boolean res = mDatabase.insertBox(POEditText.getText().toString(),BoxEditText.getText().toString(),recordSeq,simpleDateFormat.format(date));
                        if( res ){
                            //Sound successfully
                            String record = mDatabase.queryPObyBoxCode(boxcode);
                            console.append("添加 :\n"+recordSeq+",   "+ record+"\n");
                            Boxcount++;
                            BoxCounter.setText(Boxcount+"");
                            recordSeq ++;
                        }else{
                            //Sound error
                            console.append("添加失败\n");
                        }
                    }
                    BoxEditText.setText("");
                }

            }
            @Override
            public void afterTextChanged(Editable s) {

            }
        });



        ZXingLibrary.initDisplayOpinion(this);
        scanButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Intent intent = new Intent(MainActivity.this, CaptureActivity.class);
                //startActivityForResult(intent, SCAN_REQUEST_CODE);

                FrameLayout fl = (FrameLayout) findViewById(R.id.rrc_zxing_container);
                if( startCamera) {
                    startCamera = false;
                    captureFragment.onPause();
                    captureFragment.onDestroy();
                    captureFragment = null;
                    fl.setVisibility(View.GONE);
                }else{
                    startCamera = true;
                    fl.setVisibility(View.VISIBLE);
                    captureFragment = new CaptureFragment();
                    //CodeUtils.setFragmentArgs(captureFragment, R.layout.my_scaner_layout);
                    captureFragment.setAnalyzeCallback(analyzeCallback);
                    getSupportFragmentManager().beginTransaction().replace(R.id.rrc_zxing_container, captureFragment).commit();
                }


            }
        });


        // open the database
        mContext  = this.getApplicationContext();
        String dbFile=getExStorageFilePath(mDataDir,mDataFileName+".db");
        if ( dbFile.length() > 0) {
            mDatabase = new DBHelper(mContext,dbFile);
            if( mDatabase == null ){
                showMessageBox("数据库打开失败!!!");
            }
        }else{
            showMessageBox("手机存储区打开失败!!!");
            mDatabase = null;
        }


        //检查权限
        //checkPermissions();


    }

    CodeUtils.AnalyzeCallback analyzeCallback = new CodeUtils.AnalyzeCallback() {
        @Override
        public void onAnalyzeSuccess(Bitmap mBitmap, String result) {
            FrameLayout fl = (FrameLayout) findViewById(R.id.rrc_zxing_container);

            startCamera = false;
            captureFragment.onPause();
            captureFragment.onDestroy();
            captureFragment = null;
            fl.setVisibility(View.GONE);

            BoxEditText.setText(result);
        }

        @Override
        public void onAnalyzeFailed() {
            console.append("Scan: Failed");

            FrameLayout fl = (FrameLayout) findViewById(R.id.rrc_zxing_container);
            startCamera = false;
            captureFragment.onPause();
            captureFragment.onDestroy();
            captureFragment = null;
            fl.setVisibility(View.GONE);
        }
    };

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent intent) {
        super.onActivityResult(requestCode, resultCode, intent);
        switch (requestCode) {
            case SCAN_REQUEST_CODE:
                if (resultCode == RESULT_OK) {
                    if(intent!=null){
                        //处理扫描结果（在界面上显示）
                        if (null != intent) {
                            Bundle bundle = intent.getExtras();
                            if (bundle == null) {
                                return;
                            }
                            if (bundle.getInt(CodeUtils.RESULT_TYPE) == CodeUtils.RESULT_SUCCESS) {
                                String result = bundle.getString(CodeUtils.RESULT_STRING);
                                BoxEditText.setText(result);
                            } else if (bundle.getInt(CodeUtils.RESULT_TYPE) == CodeUtils.RESULT_FAILED) {
                                showMessageBox("解析二维码失败");
                            }
                        }
                    }
                }
                break;
        }
    }

    private String getExStorageFilePath(String Dirname, String Filename)
    {
        String dbFile="";
        if (Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED)) {
            dbFile = Environment.getExternalStorageDirectory().getPath() +File.separator +Dirname;
            File dbp = new File(dbFile);
            if(!dbp.exists()){
                dbp.mkdirs();
            }
            dbFile = dbFile+File.separator +Filename;
        }
        return dbFile;
    }

    private void showMessageBox(String msg){
        AlertDialog.Builder builder =  new AlertDialog.Builder(MainActivity.this);
        builder.setTitle(msg);
        builder.setPositiveButton("确定",null);
        builder.show();
    }
    private  boolean writeExternalFileBytes(String filename, byte[] bytes)
    {
        try
        {
            if (Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED))
            {
                //打开文件输入流
                File file = new File(filename);
                FileOutputStream outputStream =   new FileOutputStream(file);// openFileOutput(filename, Context.MODE_APPEND); //new FileOutputStream(fpath);
                //outputStream.getChannel().position(pos);
                outputStream.write(bytes);
                outputStream.flush();
                outputStream.close();
                return true;
            }
            Log.e("Ruan","Could not get External Storage");
            return false;
        }catch (Exception e) {
            Log.e("Ruan","error in read file: "+e.toString());
            e.printStackTrace();
            return false;
        }
    }


    @Override
    protected void onResume() {
        super.onResume();
        Log.e("Ruan","onResume");
    }

    @Override
    protected void onDestroy() {
        Log.e("Ruan","onDestroy");
        if( mDatabase != null ) mDatabase.close();
        super.onDestroy();
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

    private void checkPermissions()
    {
        String[] permissions = new String[]{
                Manifest.permission.WRITE_EXTERNAL_STORAGE,
                Manifest.permission.READ_EXTERNAL_STORAGE,
                Manifest.permission.CAMERA
        };
        List<String> data = new ArrayList<>();
        for (String permission : permissions) {
            int checkSelfPermission = ContextCompat.checkSelfPermission(this, permission);
            if(checkSelfPermission == PackageManager.PERMISSION_DENIED){//未申请
                data.add(permission);
            }
        }
        //ActivityCompat.requestPermissions(this, data.toArray(new String[data.size()]), 100);
    }


}





