plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.plugin.compose")
}

android {
    namespace = "com.zedensix.gt86dash"
    compileSdk = 37

    defaultConfig {
        applicationId = "com.zedensix.gt86dash"
        // 33: Android 13, its Bluetooth calls without the deprecated ones.
        minSdk = 33
        targetSdk = 36
        versionCode = 1
        versionName = "0.1.0"
    }

    // The phone's pages: Jetpack Compose, Material 3.
    buildFeatures {
        compose = true
    }

    buildTypes {
        release {
            isMinifyEnabled = true
            isShrinkResources = true
            proguardFiles(getDefaultProguardFile("proguard-android-optimize.txt"))
        }
    }
}

kotlin {
    compilerOptions {
        allWarningsAsErrors = true
    }
}

dependencies {
    implementation("androidx.car.app:app:1.7.0")
    implementation(platform("androidx.compose:compose-bom:2026.09.00"))
    implementation("androidx.compose.material3:material3")
    implementation("androidx.activity:activity-compose:1.12.4")
    // The drives' map (DrivesPage), OpenFreeMap's tiles: no key, no account.
    implementation("org.maplibre.gl:android-sdk:13.7.1")
    testImplementation("junit:junit:4.13.2")
}
