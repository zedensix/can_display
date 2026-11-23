plugins {
    id("com.android.application")
}

android {
    namespace = "com.zedensix.gt86dash"
    compileSdk = 37

    defaultConfig {
        applicationId = "com.zedensix.gt86dash"
        // 30: Android 11, the window insets' types (the safe area).
        minSdk = 30
        targetSdk = 36
        versionCode = 1
        versionName = "0.1.0"
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
}
