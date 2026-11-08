plugins {
    alias(libs.plugins.android.application)
    alias(libs.plugins.compose.compiler)
}

// AGP 9.x：Kotlin 编译内置，无需 kotlin-android 插件。
// 原 kotlinOptions { jvmTarget = "..." } 已失效，改用 kotlin.compilerOptions。
android {
    namespace = "com.kivo.cinema"
    compileSdk = 37
    ndkVersion = "30.0.16248370"

    defaultConfig {
        applicationId = "com.kivo.cinema"
        minSdk = 26
        targetSdk = 37
        versionCode = 1
        versionName = "0.1.0-p12a"

        ndk {
            abiFilters += listOf("arm64-v8a")
        }

        externalNativeBuild {
            cmake {
                arguments += listOf(
                    "-DANDROID_STL=c++_static",
                    "-DANDROID_PLATFORM=android-26"
                )
                cppFlags += listOf("-std=c++23", "-fvisibility=hidden")
            }
        }
    }

    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }

    // ── 签名固定在仓库内（2026-10-30）──
    //
    // 不固定时的真实事故：这台机器上有四个 Windows 账户（Administrator /
    // CodexAlt / codex / huyasi），各自有一份 `~/.android/debug.keystore`。
    // Android 的 debug 构建用**当前账户**那把私钥签名，于是同一份代码、
    // 同一个 applicationId，不同助手构建出的 APK 签名不同，`adb install -r`
    // 直接 `INSTALL_FAILED_UPDATE_INCOMPATIBLE` —— 现象看起来像"项目包被换了"，
    // 实际只是密钥换了。
    //
    // 固定之后：谁构建都签同一把钥匙，覆盖安装永远成立，不再需要 uninstall。
    // 这是 debug 专用钥匙（storePassword 就是 `android`），只用于自测装机。
    signingConfigs {
        create("kivoDebug") {
            storeFile = file("../keystore/kivo-debug.jks")
            storePassword = "android"
            keyAlias = "androiddebugkey"
            keyPassword = "android"
        }
    }

    buildTypes {
        debug {
            isMinifyEnabled = false
            // 固定签名（见上面 signingConfigs 的注释）：不这么做，
            // 换一个 Windows 账户构建就会因为 debug 密钥不同而装不上。
            signingConfig = signingConfigs.getByName("kivoDebug")
        }
        release {
            isMinifyEnabled = false
            externalNativeBuild {
                cmake {
                    // P16-P0 / D3：release 不背取证用的进程级分配器拦截。
                    // debug 不变（保持 ON），所以所有真机取证口径逐位不变。
                    arguments += listOf(
                        "-DCMAKE_BUILD_TYPE=RelWithDebInfo",
                        "-DKIVO_ENABLE_GLOBAL_ALLOC_HOOK=OFF"
                    )
                }
            }
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }

    kotlin {
        compilerOptions {
            jvmTarget.set(org.jetbrains.kotlin.gradle.dsl.JvmTarget.JVM_17)
        }
    }

    buildFeatures {
        prefab = false
        buildConfig = true
        compose = true
    }
}

dependencies {
    implementation(libs.androidx.core.ktx)
    implementation(libs.androidx.appcompat)
    // 213：Compose 播放页骨架（BOM 统一版本，material3 + activity-compose）
    val composeBom = platform("androidx.compose:compose-bom:2024.12.01")
    implementation(composeBom)
    implementation("androidx.compose.ui:ui")
    implementation("androidx.compose.ui:ui-tooling-preview")
    implementation("androidx.compose.material3:material3")
    implementation("androidx.activity:activity-compose:1.9.3")
    implementation("androidx.webkit:webkit:1.12.1")
    // 220：SMB 客户端（jcifs-ng，AC0 spike 选型；smbj 备选见设计稿 D1）
    implementation("org.codelibs:jcifs:2.1.38")
    // 220 诊断：jcifs 内部 SMB2 报文日志（slf4j-simple 落文件，收口前移除）
    implementation("org.slf4j:slf4j-simple:1.7.30")
}
