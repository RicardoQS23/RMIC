package com.example.assignment1gr20;

import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import com.google.firebase.database.DataSnapshot;
import com.google.firebase.database.DatabaseError;
import com.google.firebase.database.DatabaseReference;
import com.google.firebase.database.FirebaseDatabase;
import com.google.firebase.database.ValueEventListener;
import android.util.Log;
import android.widget.TextView;



public class LoginActivity extends AppCompatActivity {

    private EditText editTextUsername;
    private EditText editTextPassword;
    private Button buttonLogin;

    private DatabaseReference databaseReference;
    private TextView textViewError;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_login);

        // Initialize Firebase Database reference
        databaseReference = FirebaseDatabase.getInstance().getReference();

        // Initialize EditText and Button
        editTextUsername = findViewById(R.id.editTextUsername);
        editTextPassword = findViewById(R.id.editTextPassword);
        buttonLogin = findViewById(R.id.buttonLogin);
        textViewError = findViewById(R.id.textViewError);


        // Set onClickListener for login button
        buttonLogin.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {

                login();
            }
        });
    }

    // Method to perform login logic
    private void login() {
        // Get the username and password entered by the user
        final String username = editTextUsername.getText().toString().trim();
        final String password = editTextPassword.getText().toString().trim();

        // Query Firebase Database to retrieve the password associated with the entered username
        databaseReference.addListenerForSingleValueEvent(new ValueEventListener() {
            @Override
            public void onDataChange(@NonNull DataSnapshot dataSnapshot) {
                // Check if the dataSnapshot is not null and has children
                if (dataSnapshot != null && dataSnapshot.exists() && dataSnapshot.getChildrenCount() > 0) {
                    // Check if the username exists in the database
                    if (dataSnapshot.child("username").exists() && dataSnapshot.child("password").exists()) {
                        // Get the stored username and password
                        String storedUsername = dataSnapshot.child("username").getValue(String.class);
                        String storedPassword = dataSnapshot.child("password").getValue(String.class);
                        // Check if the retrieved username and password match the entered credentials
                        if (storedUsername != null && storedPassword != null && storedUsername.equals(username) && storedPassword.equals(password)) {
                            // Username and password match, login successful
                            Toast.makeText(LoginActivity.this, "Login successful", Toast.LENGTH_SHORT).show();
                            // Proceed to the next activity (MainActivity)
                            startActivity(new Intent(LoginActivity.this, MainActivity.class));
                            finish(); // Finish LoginActivity to prevent user from going back to it
                        } else {
                            // Incorrect password, show error message
                            textViewError.setText("Incorrect username or password ");
                            textViewError.setVisibility(View.VISIBLE); }
                    }
                } else {
                    // No data found in the database
                    Toast.makeText(LoginActivity.this, "No data found in the database", Toast.LENGTH_SHORT).show();
                }
            }

            @Override
            public void onCancelled(@NonNull DatabaseError databaseError) {
                // Handle database error
                Toast.makeText(LoginActivity.this, "Database error: " + databaseError.getMessage(), Toast.LENGTH_SHORT).show();
                Log.e("LoginActivity", "Database error: " + databaseError.getMessage());
            }
        });
    }
}