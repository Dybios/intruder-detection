package com.myproject.bikebt;

import static android.content.Context.BIND_AUTO_CREATE;
import static android.content.Context.LOCATION_SERVICE;

import android.content.ComponentName;
import android.content.Intent;
import android.content.ServiceConnection;
import android.location.LocationManager;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import android.os.IBinder;

import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;

public class HomeFragment extends Fragment {
    LocationService myService;
    static boolean status;
    LocationManager locationManager;
    static TextView dist, speed;
    static int p = 0;

    public HomeFragment() {
        // Required empty public constructor
    }

    private ServiceConnection sc = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            LocationService.LocalBinder binder = (LocationService.LocalBinder) service;
            myService = binder.getService();
            status = true;
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {
            status = false;
        }
    };

    void bindService() {
        if (status == true)
            return;
        Intent i = new Intent(getActivity().getApplicationContext(), LocationService.class);
        getActivity().bindService(i, sc, BIND_AUTO_CREATE);
        status = true;
    }

    void unbindService() {
        if (status == false)
            return;
        Intent i = new Intent(getActivity().getApplicationContext(), LocationService.class);
        getActivity().unbindService(sc);
        status = false;
    }

    @Override
    public void onResume() {
        super.onResume();
    }

    @Override
    public void onStart() {
        super.onStart();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        if (status == true)
            unbindService();
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        // Inflate the layout for this fragment
        return inflater.inflate(R.layout.fragment_home, container, false);
    }

    @Override
    public void onViewCreated(View view, @Nullable Bundle savedInstanceState) {
        dist = (TextView) getView().findViewById(R.id.distancetext);
        speed = (TextView) getView().findViewById(R.id.speedtext);

        locationManager = (LocationManager) getActivity().getSystemService(LOCATION_SERVICE);
        if (!locationManager.isProviderEnabled(LocationManager.GPS_PROVIDER)) {
            return;
        }

        if (status == false)
            //Here, the Location Service gets bound and the GPS Speedometer gets Active.
            bindService();
    }
}
