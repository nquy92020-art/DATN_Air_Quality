const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Cấu hình hệ thống chất lượng không khí</title>
  <style>
    * {
      box-sizing: border-box;
    }

    body {
      margin: 0;
      font-family: Arial, Helvetica, sans-serif;
      background: linear-gradient(135deg, #eef6ff, #f7fbff);
      color: #1f2937;
    }

    .page {
      max-width: 760px;
      margin: 0 auto;
      padding: 20px 14px 32px;
    }

    .hero {
      background: linear-gradient(135deg, #0f4c81, #1677b8);
      color: white;
      border-radius: 18px;
      padding: 24px 20px;
      margin-bottom: 16px;
      box-shadow: 0 10px 28px rgba(0, 0, 0, 0.12);
    }

    .hero h1 {
      margin: 0 0 8px;
      font-size: 24px;
      line-height: 1.3;
    }

    .hero p {
      margin: 0;
      opacity: 0.9;
      font-size: 14px;
    }

    .badge {
      display: inline-block;
      margin-top: 14px;
      padding: 7px 12px;
      border-radius: 999px;
      background: rgba(255, 255, 255, 0.18);
      font-size: 13px;
      font-weight: bold;
    }

    .card {
      background: white;
      border-radius: 16px;
      padding: 20px;
      margin-bottom: 16px;
      box-shadow: 0 7px 24px rgba(0, 0, 0, 0.08);
    }

    .card-title {
      margin: 0 0 16px;
      font-size: 20px;
      color: #0f4c81;
    }

    .field {
      margin-bottom: 15px;
    }

    label {
      display: block;
      margin-bottom: 7px;
      font-weight: 700;
      font-size: 14px;
    }

    input {
      width: 100%;
      height: 48px;
      padding: 0 13px;
      border: 1px solid #cbd5e1;
      border-radius: 10px;
      font-size: 16px;
      outline: none;
      background: #fbfdff;
    }

    input:focus {
      border-color: #1677b8;
      box-shadow: 0 0 0 3px rgba(22, 119, 184, 0.12);
      background: white;
    }

    .hint {
      margin: 6px 0 0;
      font-size: 12px;
      color: #64748b;
      line-height: 1.45;
    }

    .threshold-block {
      padding: 14px;
      margin-bottom: 13px;
      border-radius: 12px;
      background: #f8fafc;
      border: 1px solid #e2e8f0;
    }

    .threshold-title {
      margin: 0 0 5px;
      font-size: 15px;
      font-weight: 700;
    }

    .row {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 10px;
      margin-top: 10px;
    }

    .actions {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 10px;
      margin-top: 18px;
    }

    button {
      min-height: 50px;
      border: 0;
      border-radius: 11px;
      font-size: 16px;
      font-weight: 700;
      cursor: pointer;
    }

    #btnDefauld {
      background: #e8f1f8;
      color: #0f4c81;
      border: 1px solid #b8d2e6;
    }

    #btnSubmit {
      background: #0f7a5a;
      color: white;
    }

    button:active {
      transform: scale(0.99);
    }

    .footer {
      text-align: center;
      font-size: 12px;
      color: #64748b;
      padding-top: 4px;
    }

    @media (max-width: 520px) {
      .hero h1 {
        font-size: 20px;
      }

      .card {
        padding: 16px;
      }

      .row,
      .actions {
        grid-template-columns: 1fr;
      }
    }
  </style>
</head>

<body>
  <div class="page">

    <section class="hero">
      <h1>Hệ thống giám sát &amp; kiểm soát chất lượng không khí</h1>
      <p>Trang cấu hình ESP32 cho Wi-Fi, Blynk và các ngưỡng điều khiển.</p>
      <div class="badge">ESP32 • Wi-Fi • Blynk IoT</div>
    </section>

    <section class="card">
      <h2 class="card-title">1. Kết nối mạng</h2>

      <div class="field">
        <label for="ssid">Tên Wi-Fi</label>
        <input type="text" id="ssid" name="ssid" placeholder="Ví dụ: TP-LINK_2B70">
      </div>

      <div class="field">
        <label for="pass">Mật khẩu Wi-Fi</label>
        <input type="text" id="pass" name="pass" placeholder="Nhập mật khẩu Wi-Fi">
      </div>

      <div class="field">
        <label for="token">Blynk Auth Token</label>
        <input type="text" id="token" name="token" placeholder="Nhập Auth Token của thiết bị">
        <p class="hint">Auth Token được dùng để kết nối ESP32 với thiết bị trên Blynk Cloud.</p>
      </div>
    </section>

    <section class="card">
      <h2 class="card-title">2. Ngưỡng môi trường</h2>

      <div class="threshold-block">
        <p class="threshold-title">Nhiệt độ (&deg;C)</p>
        <p class="hint">Ngưỡng thấp &lt; vùng an toàn &lt; ngưỡng cao</p>
        <div class="row">
          <input type="number" id="tempThreshold1" name="tempThreshold1"
                 min="0" max="100" step="1" placeholder="Ngưỡng thấp">
          <input type="number" id="tempThreshold2" name="tempThreshold2"
                 min="0" max="100" step="1" placeholder="Ngưỡng cao">
        </div>
      </div>

      <div class="threshold-block">
        <p class="threshold-title">Độ ẩm (%)</p>
        <p class="hint">Ngưỡng thấp &lt; vùng an toàn &lt; ngưỡng cao</p>
        <div class="row">
          <input type="number" id="humiThreshold1" name="humiThreshold1"
                 min="0" max="100" step="1" placeholder="Ngưỡng thấp">
          <input type="number" id="humiThreshold2" name="humiThreshold2"
                 min="0" max="100" step="1" placeholder="Ngưỡng cao">
        </div>
      </div>

      <div class="threshold-block">
        <p class="threshold-title">Bụi PM2.5 (&micro;g/m&sup3;)</p>
        <p class="hint">Giá trị PM2.5 được dùng để đánh giá và điều khiển thiết bị lọc không khí.</p>
        <div class="row">
          <input type="number" id="dustThreshold1" name="dustThreshold1"
                 min="0" max="500" step="1" placeholder="Ngưỡng 1">
          <input type="number" id="dustThreshold2" name="dustThreshold2"
                 min="0" max="500" step="1" placeholder="Ngưỡng 2">
        </div>
      </div>

      <div class="actions">
        <button id="btnDefauld" type="button">Khôi phục mặc định</button>
        <button id="btnSubmit" type="button">Lưu cấu hình</button>
      </div>
    </section>

    <div class="footer">Indoor Air Quality Monitoring &amp; Control System</div>
  </div>

  <script>
    var data = {
      ssid: "",
      pass: "",
      token: "",
      tempThreshold1: "",
      tempThreshold2: "",
      humiThreshold1: "",
      humiThreshold2: "",
      dustThreshold1: "",
      dustThreshold2: ""
    };

    const ssid = document.getElementById("ssid");
    const pass = document.getElementById("pass");
    const token = document.getElementById("token");

    const tempThreshold1 = document.getElementById("tempThreshold1");
    const tempThreshold2 = document.getElementById("tempThreshold2");
    const humiThreshold1 = document.getElementById("humiThreshold1");
    const humiThreshold2 = document.getElementById("humiThreshold2");
    const dustThreshold1 = document.getElementById("dustThreshold1");
    const dustThreshold2 = document.getElementById("dustThreshold2");

    const btnDefauld = document.getElementById("btnDefauld");
    const btnSubmit = document.getElementById("btnSubmit");

    var xhttp = new XMLHttpRequest();
    xhttp.open("GET", "/data_before", true);
    xhttp.send();

    xhttp.onreadystatechange = function() {
      if (xhttp.readyState == 4 && xhttp.status == 200) {
        const obj = JSON.parse(xhttp.responseText);

        ssid.value = obj.ssid;
        pass.value = obj.pass;
        token.value = obj.token;
        tempThreshold1.value = obj.tempThreshold1;
        tempThreshold2.value = obj.tempThreshold2;
        humiThreshold1.value = obj.humiThreshold1;
        humiThreshold2.value = obj.humiThreshold2;
        dustThreshold1.value = obj.dustThreshold1;
        dustThreshold2.value = obj.dustThreshold2;
      }
    };

    btnDefauld.addEventListener("click", function() {
      tempThreshold1.value = 20;
      tempThreshold2.value = 32;
      humiThreshold1.value = 40;
      humiThreshold2.value = 75;
      dustThreshold1.value = 40;
      dustThreshold2.value = 150;
    });

    btnSubmit.addEventListener("click", function() {
      data = {
        ssid: ssid.value,
        pass: pass.value,
        token: token.value,
        tempThreshold1: Number(tempThreshold1.value),
        tempThreshold2: Number(tempThreshold2.value),
        humiThreshold1: Number(humiThreshold1.value),
        humiThreshold2: Number(humiThreshold2.value),
        dustThreshold1: Number(dustThreshold1.value),
        dustThreshold2: Number(dustThreshold2.value)
      };

      var xhttp2 = new XMLHttpRequest();
      xhttp2.open("POST", "/post_data", true);
      xhttp2.send(JSON.stringify(data));

      xhttp2.onreadystatechange = function() {
        if (xhttp2.readyState == 4 && xhttp2.status == 200) {
          alert("Đã lưu cấu hình. ESP32 sẽ khởi động lại.");
        }
      };
    });
  </script>
</body>
</html>
)rawliteral";
