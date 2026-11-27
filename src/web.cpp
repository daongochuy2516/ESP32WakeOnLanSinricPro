#include "web.h"

const char* homePage = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Device config</title>
    <style>
        body {
            margin: 0;
            font-family: Arial, sans-serif;
            background-color: #f4f4f4;
            display: flex;
            flex-direction: column;
            align-items: center;
            padding: 20px;
        }

        h2 {
            color: #333;
            text-align: center;
            margin-bottom: 20px;
        }

        form {
            width: 100%;
            max-width: 400px;
            background-color: white;
            padding: 20px;
            border-radius: 8px;
            box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
        }

        label {
            display: block;
            margin-bottom: 8px;
            color: #555;
        }

        input[type="text"] {
            width: 100%;
            padding: 10px;
            margin-bottom: 20px;
            border: 1px solid #ccc;
            border-radius: 4px;
            font-size: 16px;
            box-sizing: border-box;
        }

        input[type="submit"] {
            width: 100%;
            padding: 12px;
            border: none;
            background-color: #4CAF50;
            color: white;
            font-size: 16px;
            border-radius: 4px;
            cursor: pointer;
        }

        input[type="submit"]:hover {
            background-color: #45a049;
        }

        footer {
            margin-top: 20px;
            text-align: center;
            font-size: 0.8rem;
            color: #777;
        }

        footer p {
            margin: 5px 0;
        }
        @media (max-width: 600px) {
            body {
                padding: 15px;
            }
            h2 {
                font-size: 20px;
            }
            form {
                padding: 15px;
            }
            input[type="text"], input[type="submit"] {
                font-size: 14px;
            }
        }
            .toggle-group {
        display: flex;
        align-items: center;
        justify-content: space-between;
        margin-bottom: 16px;
        font-family: sans-serif;
    }

    .toggle-group label {
        font-weight: 600;
        font-size: 16px;
    }

    .switch {
        position: relative;
        display: inline-block;
        width: 46px;
        height: 24px;
    }

    .switch input {
        opacity: 0;
        width: 0;
        height: 0;
    }

    .slider {
        position: absolute;
        cursor: pointer;
        background-color: #ccc;
        border-radius: 34px;
        top: 0;
        left: 0;
        right: 0;
        bottom: 0;
        transition: 0.3s;
    }

    .slider:before {
        position: absolute;
        content: "";
        height: 18px;
        width: 18px;
        left: 3px;
        bottom: 3px;
        background-color: white;
        border-radius: 50%;
        transition: 0.3s;
    }

    .switch input:checked + .slider {
        background-color: #4CAF50;
    }

    .switch input:checked + .slider:before {
        transform: translateX(22px);
    }
    .form-group {
    margin-bottom: 20px;
    display: flex;
    flex-direction: column;
    }

    label {
    font-weight: bold;
    margin-bottom: 8px;
    }

    .custom-select {
    padding: 10px;
    font-size: 14px;
    border-radius: 5px;
    border: 1px solid #ccc;
    outline: none;
    transition: border-color 0.3s ease;
    }

    .custom-select:focus {
    border-color: #4CAF50;
    }

    small.note {
    font-size: 12px;
    color: #888;
    margin-top: 5px;
    }

    .custom-select:hover {
    background-color: #f0f0f0;
    }

    </style>
</head>
<body>

    <h2>ESP32 - SinricPro WakeOnLan device config</h2>

    <form action="/save" method="POST">
        <label for="ssid">Wi-Fi name (ONLY 2.4GHz pls):</label>
        <input type="text" id="ssid" name="ssid" placeholder="%WIFI%">

        <label for="password">Wi-Fi Password:</label>
        <input type="text" id="password" name="password" placeholder="%WIFIPASS%">

        <label for="appKey">APP KEY:</label>
        <input type="text" id="appKey" name="appKey" placeholder="%APPKEY%">

        <label for="appSecret">APP SECRET:</label>
        <input type="text" id="appSecret" name="appSecret" placeholder="%APPSECRET%">

        <label for="deviceId">DEVICE ID:</label>
        <input type="text" id="deviceId" name="deviceId" placeholder="%DEVICEID%">

        <label for="pcMac">PC MAC Address:</label>
        <input type="text" id="pcMac" name="pcMac" placeholder="%PCMAC%">

        <div class="form-group">
            <label for="wolMode">Wake-on-LAN Mode:</label>
            <select id="wolMode" name="wolMode" class="custom-select">
              <option value="both" %WOL_MODE_BOTH% >Allow both</option>
              <option value="sinric" %WOL_MODE_SINRIC% >SinricPro only</option>
              <option value="physical" %WOL_MODE_PHYSICAL% >Physical button only</option>
            </select>
            <small class="note">Choose how the PC can be turned on.</small>
          </div>
           
          
        <div class="toggle-group">
            <label for="enableLed">Enable LED RGB Effect</label>
            <label class="switch">
              <input type="checkbox" id="enableLed" name="enableLed" %ENABLE_LED%>
              <span class="slider"></span>
            </label>
          </div>
          
          <div class="toggle-group">
            <label for="enableBuzzer">Enable Buzzer</label>
            <label class="switch">
              <input type="checkbox" id="enableBuzzer" name="enableBuzzer" %ENABLE_BUZZER% >
              <span class="slider"></span>
            </label>
          </div>
          
        <input type="submit" value="Save">
    </form>

    <footer>
        <p>You must connect this device to your own SinricPro account for it to function properly.<br>
        Otherwise, the device will not work correctly!<br>
        Firmware Version: %firmwareVersion%<br>
        Copyright © daongochuy2516
        </p>
    </footer>

</body>
</html>

)rawliteral";

const char* loadPage = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0"/>
  <title>Restarting...</title>
  <style>
    body {
      margin: 0;
      font-family: Arial, sans-serif;
      background-color: #f4f4f4;
      display: flex;
      justify-content: center;
      align-items: center;
      height: 100vh;
    }

    .container {
      background-color: #fff;
      padding: 30px 40px;
      border-radius: 12px;
      box-shadow: 0 4px 15px rgba(0, 0, 0, 0.1);
      text-align: center;
      max-width: 400px;
      width: 100%;
    }

    .success {
      font-size: 20px;
      font-weight: bold;
      color: #4CAF50;
      margin-bottom: 10px;
    }

    .message {
      font-size: 15px;
      color: #555;
      margin-bottom: 25px;
    }

    .progress-bar {
      position: relative;
      height: 12px;
      width: 100%;
      background-color: #e0e0e0;
      border-radius: 6px;
      overflow: hidden;
      box-shadow: inset 0 2px 5px rgba(0, 0, 0, 0.1);
    }

    .indeterminate {
      position: absolute;
      height: 100%;
      background-color: #4CAF50;
      animation: fillBar 3s forwards;
      border-radius: 6px;
    }

    @keyframes fillBar {
      0% {
        left: 0;
        width: 0;
      }
      100% {
        left: 0;
        width: 100%;
      }
    }

  </style>
</head>
<body>
  <div class="container">
    <div class="success">Success!</div>
    <div class="message">Settings have been saved on this device. <br>Please wait while the device restarts...</div>
    <div class="progress-bar">
      <div class="indeterminate"></div>
    </div>
  </div>
</body>
</html>

)rawliteral";