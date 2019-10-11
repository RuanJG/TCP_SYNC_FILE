package com.rompa.jk.tcpfilesyncer;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.DatabaseErrorHandler;
import android.database.SQLException;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.util.Log;

/**
 * Created by RSL-030 on 2019/10/8.
 */

public class DBHelper {
    private static final String TAG = "DBHelper";

    public static  String DATABASE_NAME = "RRCBoxBarcodeList.db";
    private static final int DATABASE_VERSION = 1;

    public static final String TABLE_NAME = "BoxBarcodeList";
    public static final String COLUMN_BOX_CODE = "BoxCode";
    public static final String COLUMN_PO_CODE = "POCode";
    public static final String COLUMN_SEQ = "SequenceNo";
    public static final String COLUMN_DATE = "Date";
    public static final String[] COLUMN_LIST={
        COLUMN_BOX_CODE,
                COLUMN_PO_CODE,
                COLUMN_SEQ,
                COLUMN_DATE
    };

    private SQLiteDatabase mDatabase;
    private Context mContext;

    public DBHelper(Context context, String DBFile){
        mContext = context;
        DATABASE_NAME  = DBFile;
        mDatabase = new SQLHelper(mContext).getWritableDatabase();
    }
    public void close()
    {
        mDatabase.close();
    }

    public String queryPObyBoxCode(String BoxCode)
    {
        Cursor cursor;
        String PONum = "";

        cursor = mDatabase.query(TABLE_NAME,COLUMN_LIST, COLUMN_BOX_CODE+" = ?",new String[]{BoxCode},null,null,null);

        try {
            if( cursor != null && cursor.getCount() > 0 && cursor.moveToNext()) {
                PONum = //cursor.getString(cursor.getColumnIndex(COLUMN_SEQ))+" ,  "+
                        cursor.getString(cursor.getColumnIndex(COLUMN_BOX_CODE))+" ,  "+
                        //cursor.getString(cursor.getColumnIndex(COLUMN_PO_CODE))+" ,  "+
                        cursor.getString(cursor.getColumnIndex(COLUMN_DATE));
            }
        }catch (SQLException e) {
            Log.e(TAG,"queryPObyBoxCode queryData exception", e);
        }
        return PONum;
    }

    public String[] getPOBoxList(String PO){
        Cursor cursor;
        String[] boxList = new String[0];

        cursor = mDatabase.query(TABLE_NAME,COLUMN_LIST, COLUMN_PO_CODE+" = ?",new String[]{PO},null,null,null);
        try{
            if( cursor != null && cursor.getCount() > 0){
                int cnt = cursor.getCount();
                boxList = new String[cnt];
                for( int i=0 ; i< cnt; i++){
                    if( cursor.moveToNext()) {
                        //TODO fill result in boxList
                        boxList[i]= //cursor.getString(cursor.getColumnIndex(COLUMN_SEQ))+" ,  "+
                                cursor.getString(cursor.getColumnIndex(COLUMN_BOX_CODE))+" ,  "+
                                //cursor.getString(cursor.getColumnIndex(COLUMN_PO_CODE))+" ,  "+
                                cursor.getString(cursor.getColumnIndex(COLUMN_DATE));
                    }
                }
            }
        }catch (SQLException e) {
            Log.e(TAG,"queryPObyBoxCode queryData exception", e);
        }

        return boxList;
    }

    public byte[] getAllRecord(){
        Cursor cursor;
        String boxList ="";

        cursor = mDatabase.query(TABLE_NAME,COLUMN_LIST, null,null,null,null,null);
        try{
            if( cursor != null && cursor.getCount() > 0){
                int cnt = cursor.getCount();
                for( int i=0 ; i< cnt; i++){
                    if( cursor.moveToNext()) {
                        //TODO fill result in boxList
                        boxList += (cursor.getString(cursor.getColumnIndex(COLUMN_SEQ))+","+
                                cursor.getString(cursor.getColumnIndex(COLUMN_BOX_CODE))+","+
                                        cursor.getString(cursor.getColumnIndex(COLUMN_PO_CODE))+","+
                                        cursor.getString(cursor.getColumnIndex(COLUMN_DATE))+"\n\r");
                    }
                }
            }
        }catch (SQLException e) {
            Log.e(TAG,"getAllRecord queryData exception", e);
        }

        return boxList.getBytes();
    }

    public boolean insertBox(String PO, String Barcode, int seq, String date){
        ContentValues contentValues = new ContentValues();
        contentValues.put(COLUMN_BOX_CODE, Barcode);
        contentValues.put(COLUMN_PO_CODE,PO);
        contentValues.put(COLUMN_SEQ, seq);
        contentValues.put(COLUMN_DATE, date);

        if( -1 == mDatabase.insert(TABLE_NAME, null,contentValues) )
            return false;
        return true;
    }

    public boolean replaceBox(String PO, String Barcode, int seq, String date){
        ContentValues contentValues = new ContentValues();
        contentValues.put(COLUMN_BOX_CODE, Barcode);
        contentValues.put(COLUMN_PO_CODE,PO);
        contentValues.put(COLUMN_SEQ, seq);
        contentValues.put(COLUMN_DATE, date);

        //mDatabase.update(DBHelper.TABLE_NAME,contentValues,COLUMN_BOX_CODE+" = ?",new String[]{boxCode});
        if( -1 == mDatabase.insertWithOnConflict(TABLE_NAME, null,contentValues, SQLiteDatabase.CONFLICT_REPLACE) )
            return false;
        return true;
    }

    public  boolean deleteBox( String boxCode){
        if( 1 == mDatabase.delete(TABLE_NAME,COLUMN_BOX_CODE+" = ?",new String[]{boxCode}) )
            return true;
        return false;
    }
















    public class SQLHelper extends SQLiteOpenHelper {
        private static final String TAG = "SQLHelper";

        public SQLHelper(Context context) {
            this(context, DATABASE_NAME, null, DATABASE_VERSION);
        }

        public SQLHelper(Context context, String name, SQLiteDatabase.CursorFactory factory, int version) {
            super(context, DATABASE_NAME, factory, DATABASE_VERSION);
        }

        public SQLHelper(Context context, String name, SQLiteDatabase.CursorFactory factory, int version, DatabaseErrorHandler errorHandler) {
            super(context, DATABASE_NAME, factory, DATABASE_VERSION, errorHandler);
        }

        @Override
        public void onCreate(SQLiteDatabase database) {
            String sql = "CREATE TABLE IF NOT EXISTS "
                    + TABLE_NAME + " ( "
                    + COLUMN_BOX_CODE + " TEXT PRIMARY KEY , " //AUTOINCREMENT
                    + COLUMN_PO_CODE + " TEXT,"
                    + COLUMN_SEQ + " INTEGER, "
                    + COLUMN_DATE + " TEXT)";
            try {
                database.execSQL(sql);
            }catch (SQLException e) {
                Log.e(TAG,"onCreate exception", e);
            }
        }

        @Override
        public void onUpgrade(SQLiteDatabase database, int oldVersion, int newVersion) {

        }
    }









}
