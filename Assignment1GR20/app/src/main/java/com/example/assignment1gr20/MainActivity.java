package com.example.assignment1gr20;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import android.content.Intent;


import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import com.google.firebase.Firebase;
import com.google.firebase.FirebaseError;
import com.google.firebase.database.DataSnapshot;
import com.google.firebase.database.DatabaseError;
import com.google.firebase.database.DatabaseReference;
import com.google.firebase.database.FirebaseDatabase;
import com.google.firebase.database.ValueEventListener;
import android.widget.ImageView;
import com.google.firebase.storage.StorageReference;
import com.google.firebase.storage.FirebaseStorage;
import com.bumptech.glide.Glide;
import android.widget.ImageView;
import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import com.google.android.gms.tasks.OnFailureListener;
import com.google.android.gms.tasks.OnSuccessListener;
import com.google.firebase.storage.FirebaseStorage;
import com.google.firebase.storage.ListResult;
import com.google.firebase.storage.StorageReference;
import java.util.List;
import android.net.Uri;
import android.os.Handler;





public class MainActivity extends AppCompatActivity {

    FirebaseDatabase database;
    // Initialize Firebase Storage

    DatabaseReference  alarmstatusReference, enableSoundReference, authenticationReference,movement_detectionReference;
    TextView alarmStatus;
    Button enableSound,gallery;

    Boolean currentSound;

    private ImageView imageView, imageView2,imageView3;
    private StorageReference storageRef;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.activity_main);

        imageView = findViewById(R.id.imageView);
        imageView2 = findViewById(R.id.imageView2);
        imageView3 = findViewById(R.id.imageView3);
        storageRef = FirebaseStorage.getInstance().getReference();

        // Call method to initially load the most recent image
        loadMostRecentImage();

        // Set up listener for changes to storage
        setUpStorageListener();
    }

    private void loadMostRecentImage() {
        // Get a reference to the "data" folder in Firebase Storage
        StorageReference photosRef = storageRef.child("data");

        photosRef.listAll().addOnSuccessListener(new OnSuccessListener<ListResult>() {
            @Override
            public void onSuccess(ListResult listResult) {
                List<StorageReference> items = listResult.getItems();

                if (!items.isEmpty()) {
                    StorageReference highestNumberImageRef = null;
                    int highestNumber = Integer.MIN_VALUE;

                    for (StorageReference item : items) {
                        String filename = item.getName();
                        int number = extractNumberFromFilename(filename);
                        Log.d("ImageLoader", "Filename: " + filename + ", Number: " + number);
                        if (number > highestNumber) {
                            highestNumber = number;
                            highestNumberImageRef = item;
                        }
                    }

                    if (highestNumberImageRef != null) {
                        highestNumberImageRef.getDownloadUrl().addOnSuccessListener(new OnSuccessListener<Uri>() {
                            @Override
                            public void onSuccess(Uri uri) {
                                Log.d("ImageLoader", "Image URI: " + uri.toString());
                                // Use Glide or Picasso to load the image into the ImageView
                                Glide.with(MainActivity.this).load(uri).into(imageView);
                            }
                        }).addOnFailureListener(new OnFailureListener() {
                            @Override
                            public void onFailure(@NonNull Exception e) {
                                // Handle any errors
                                Log.e("ImageLoader", "Error loading image: " + e.getMessage());
                            }
                        });
                    }
                } else {
                    // Handle case where there are no items in the list
                    Log.d("ImageLoader", "No images found in the 'data' folder");
                }
            }
        }).addOnFailureListener(new OnFailureListener() {
            @Override
            public void onFailure(@NonNull Exception e) {
                // Handle any errors
                Log.e("ImageLoader", "Error listing items in the 'data' folder: " + e.getMessage());
            }
        });
    }


    // Method to extract number from filename (assuming the filename format is "number.png")
    private int extractNumberFromFilename(String filename) {
        int lastIndex = filename.lastIndexOf('.');
        String numberStr = filename.substring(0, lastIndex);
        return Integer.parseInt(numberStr);
    }




    private void setUpStorageListener() {
        // Get reference to the root of your Firebase Storage
        StorageReference rootRef = FirebaseStorage.getInstance().getReference();

        // Set up a listener for changes to the root reference
        rootRef.listAll()
                .addOnSuccessListener(listResult -> {
                    // Iterate through the list of items (images)
                    for (StorageReference item : listResult.getItems()) {
                        // Listen for changes to each individual item
                        item.getDownloadUrl().addOnSuccessListener(uri -> {
                            // Handle newly added image here
                            // You might want to trigger the reload of the most recent image
                            loadMostRecentImage();
                        });
                    }
                })
                .addOnFailureListener(e -> {
                    // Handle any errors
                    Log.e("MainActivity", "Error setting up storage listener: " + e.getMessage());
                });
    }

    private Runnable imageUpdateRunnable = new Runnable() {
        @Override
        public void run() {
            loadMostRecentImage();
            handler.postDelayed(this, 50); // Check for updates
        }
    };

    private Handler handler = new Handler();

    @Override
    protected void onStart(){
        super.onStart();

        //measurement = (TextView) findViewById(R.id.measurement);
        enableSound = (Button) findViewById(R.id.button);
        gallery = (Button) findViewById(R.id.button3);
        database = FirebaseDatabase.getInstance("FIREBASE_URL"); // Add your Firebase URL here
        //firebaseReference.addValueEventListener(new ValueEventListener(){
        //measurementReference = database.getReference("measurement");

        enableSoundReference = database.getReference("enable_alarm");
        authenticationReference = database.getReference("authentication");
        alarmstatusReference = database.getReference("alarm_status");
        movement_detectionReference = database.getReference("movement_detection");
        handler.post(imageUpdateRunnable);


        authenticationReference.addValueEventListener(new ValueEventListener() {
            @Override
            public void onDataChange(@NonNull DataSnapshot snapshot) {
                Boolean value = snapshot.getValue(Boolean.class);
                if (value != null) {
                    // Update the image based on the boolean value
                    if (value) {
                        imageView2.setImageResource(R.drawable.unlocked_locker);
                    } else {
                        imageView2.setImageResource(R.drawable.locked_locker);
                    }
                }
            }

            @Override
            public void onCancelled(@NonNull DatabaseError error) {

            }
        });
        movement_detectionReference.addValueEventListener(new ValueEventListener() {
            @Override
            public void onDataChange(@NonNull DataSnapshot snapshot) {
                Boolean value = snapshot.getValue(Boolean.class);
                if (value != null) {
                    // Update the image based on the boolean value
                    if (value) {
                        imageView3.setImageResource(R.drawable.movement);
                    } else {
                        imageView3.setImageResource(R.drawable.no_movement);
                    }
                }
            }

            @Override
            public void onCancelled(@NonNull DatabaseError error) {

            }
        });
        gallery.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {

                startActivity(new Intent(MainActivity.this, GalleryActivity.class));
            }
        });

        enableSoundReference.addValueEventListener(new ValueEventListener() {
            @Override
            public void onDataChange(@NonNull DataSnapshot snapshot) {
                Boolean value = snapshot.getValue(Boolean.class);
                currentSound = value;
                enableSound.setText(value ? "Disable Alarm" : "Enable Alarm");
            }

            @Override
            public void onCancelled(@NonNull DatabaseError error) {
                currentSound = false;
            }
        });

        enableSound.setOnClickListener(new View.OnClickListener(){

            @Override
            public void onClick(View view) {
                enableSoundReference.setValue(!currentSound);
            }
        });


    }
    @Override
    protected void onStop() {
        super.onStop();
        // Stop the periodic task when the activity is stopped
        handler.removeCallbacks(imageUpdateRunnable);
    }
}
