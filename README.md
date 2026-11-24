[README.md](https://github.com/user-attachments/files/35323604/README.md)
# Valorant Sensitivity Finder

Ứng dụng luyện aim và tìm độ nhạy chuột cho VALORANT trên Windows.

## Chạy ứng dụng

1. Sao chép thư mục `Release` vào một nơi có quyền ghi trên máy Windows 10/11 x64 có trình điều khiển đồ họa hỗ trợ OpenGL 3.3. Mở `ValorantSensFinder.exe`. Bản đóng gói chạy trực tiếp và không cần Visual C++ Redistributable.
2. Trong **Benchmarks**, chọn **Full** hoặc **Condensed** để luyện từng bài. Trong **Crosshair**, chọn preset hoặc chỉnh tâm ngắm. Ở bài Tracking, đầu bot có đường kính 30 cm và bot chuyển xanh khi tâm ngắm chạm đầu hoặc thân.
3. Để tìm sens, nhập **Mouse DPI**, chọn sens hiện tại hoặc mức gợi ý **280 eDPI**, rồi bấm **Start Auto-Calibration**. Sau khi hoàn tất, **Copy Sens** sao chép sens đề xuất và **Save Results** lưu báo cáo CSV.

Auto-Calibration đếm ngược **3–2–1** trước mỗi vòng. Bật **Blind Sens** để ứng dụng chọn điểm bắt đầu bí mật trong khoảng **245–315 eDPI** và chỉ hiển thị sens cùng điểm số khi có báo cáo cuối.

Trong **Target Switching**, chỉ cần rê tâm ngắm bám theo mục tiêu. Mục tiêu đổi sang màu bám trúng của Tracking khi tâm nằm trên hitbox và trở về màu thường khi tâm rời hitbox. Ứng dụng tự bắn khi bám trúng, không tạo tia lửa. Mỗi mục tiêu có 150 HP và cần 4 phát trúng để hạ. Ở nhịp bắn 9,75 phát/giây, bốn phát liên tiếp mất khoảng 308 ms tính từ phát đầu đến phát cuối.

Chọn **1v1 Combat Arena** trong menu hoặc bấm **Try in 1v1 Arena** trên báo cáo để thử ngay sens đề xuất. Dùng **WASD** để di chuyển, giữ **Shift trái** để đi bộ, giữ **Ctrl trái** để ngồi, dùng **Space** để nhảy và **chuột trái** để bắn. Đổi phím trong tab **Controls**. Arena dùng vật lý 128 Hz với tốc độ Rifle. Khi đang nhảy, giữ Ctrl để co chân và lên thùng thấp 150 cm. Các thùng cao 220 cm dùng để che chắn. Chờ HUD báo **Accurate** sau khi hãm để bắn với độ tản thấp. **Tagged** báo thời gian tốc độ đang hồi phục sau khi trúng đạn. Bot dừng hẳn trước khi bắn. Sau mỗi lần hạ gục, số đếm ngược hiện giữa màn hình từ 1.0 đến 0.1 trước khi lượt mới bắt đầu. Cả hai đứng yên tại vị trí xuất phát trong lúc đếm. Bạn vẫn có thể xoay góc nhìn.

Trong Arena và Free Aim, bảng **Shooting Error** ghi 20 phát gần nhất, kể cả phát trượt. Phần vàng là độ giật dọc cao nhất từ đầu loạt đến phát đó. Phần xanh cyan là sai số do di chuyển của riêng phát đó. Sau 375 ms kể từ phát cuối, phát tiếp theo bắt đầu đo lại đỉnh giật dọc.

Trong Arena và Free Aim, giữ chuột trái để sấy Vandal, bấm **chuột phải** để bật hoặc tắt ADS. Cả hai chế độ đều có đạn vô hạn và có thể giữ cò qua mốc 25 viên. Camera giật theo loạt bắn và đạn để lại vết trên bề mặt trúng.

Trong một loạt Vandal đã hồi tâm, ba viên đầu gom sát, từ viên thứ tư đạn leo rõ hơn và sau viên thứ sáu bắt đầu lệch ngang. Camera chỉ nhận một phần độ giật của đạn. Nhả cò trong **375 ms** để độ giật và độ tản hồi hoàn toàn.

Chọn **Bot rank** cạnh nút **1v1 Combat Arena** hoặc trên báo cáo Auto-Calibration. Có chín rank từ **Iron** đến **Radiant**. Rank thấp kéo tâm chậm hơn và có thể bắn khi vẫn đang strafe. Rank cao đặt tâm ngang đầu thường xuyên hơn, kéo tâm nhanh hơn và phối hợp nhịp dừng, bắn, đổi hướng chặt hơn. **Ascendant** là mặc định.

Trong **1v1 Combat Arena** và **Online 1v1**, bóng mờ của đối thủ hiện qua vật cản trong giây cuối của đếm ngược. Đối thủ xuất hiện ở vị trí của round sắp đấu. Khi hết giờ, vật cản che đối thủ như bình thường.

Bot đặt tâm sẵn ở tầm đứng khi bạn ló góc và cần một nhịp để chỉnh lại nếu bạn ngồi. Qua mỗi lượt 1v1, súng giữ nhịp bắn hiện tại và người chơi giữ đà ngang cùng lệnh nhảy đã đệm.

Chọn **Free Aim** để tập di chuyển và bắn Vandal trong phòng trống. Mặc định dùng **WASD** để di chuyển, giữ **Shift trái** để đi bộ, giữ **Ctrl trái** để ngồi và bấm **Space** để nhảy. Để bunny hop, giữ **A/D** và xoay chuột cùng phía khi ở trên không, rồi bấm nhảy sát lúc tiếp đất. Bấm **Home** để đặt lại góc nhìn. **Controls** có phím chính và phím phụ cho từng hành động. Bạn có thể gán con lăn làm phím nhảy phụ.

Bot có thể đổi hướng ADAD rồi tap từng viên, ngồi sấy 4–6 viên với nhịp 180 ms hoặc strafe giữa các loạt hai viên. Đầu mỗi round, bạn và bot đứng sau hai thùng cao riêng, có lối ló ra ở cả hai bên. Đường đạn sáng giúp nhìn rõ hướng bắn. Khi bạn hạ bot, Arena phát âm báo và hiện banner **BOT ELIMINATED** bên dưới tâm ngắm. Trong **Audio**, chỉnh riêng **Kill**, **Opponent shot** và **Player down**.

Nhập sens luyện tập hoặc sens khởi đầu theo bước **0,001**, ví dụ `0.187`. **Copy Sens** làm tròn kết quả về 3 chữ số thập phân để nhập vào VALORANT. Các giá trị đo theo từng vòng được giữ nguyên trong CSV.

Ứng dụng đọc `settings.ini` khi mở và lưu DPI, sens luyện tập, thiết lập Auto-Calibration và tâm ngắm khi thoát. File cấu hình, `telemetry/` và `history/` nằm cạnh file `.exe`. Sao chép các file và thư mục này nếu muốn mang theo cấu hình và dữ liệu giữa các máy.

Ứng dụng mở ở **Fullscreen**. Tùy chọn **Low Latency** trong menu tắt VSync và giới hạn công việc dựng hình đang chờ trên GPU trong lúc tập. Tùy chọn mặc định tắt, được lưu trong `settings.ini` và có thể gây xé hình hoặc giảm FPS khi bật.

Trong **Appearance**, bạn có thể đổi màu nền và các bề mặt của phòng, mục tiêu, neo, trạng thái bám trúng và tia lửa. **Audio** cho phép tắt tiếng, chỉnh âm lượng tổng hoặc chỉnh và nghe thử từng âm bắn, bắn trúng và đếm ngược. Màu sắc và âm lượng được lưu trong `settings.ini`.

Trong **Appearance → Graphics**, chọn **Anti-aliasing (MSAA)** ở mức **Off**, **2x**, **4x** (mặc định) hoặc **8x**. Thay đổi áp dụng ngay cho phòng tập 3D. Nếu GPU không hỗ trợ mức đã chọn, ứng dụng hiển thị mức thấp hơn đang dùng.

## Online 1v1

Hai người mở tab **Online 1v1** và dùng cùng một relay. Một người bấm **Create Room** rồi gửi mã sáu ký tự cho bạn. Người kia nhập mã và bấm **Join Room**. Máy chủ phòng chạy trận ở 128 Hz và quyết định sát thương, điểm cùng thời gian hồi sinh. Nếu cả hai cùng hạ nhau trong một tick, round hòa.

Online dùng hitbox mục tiêu của Arena cho hai bên như nhau, với đầu hình cầu bán kính 14 cm và thân hình capsule bán kính 26 cm. Đầu, thân và chân lần lượt nhận 160, 40 và 34 sát thương. Host chọn vị trí xuất phát và bố trí thùng mới mỗi round. Cả hai xuất hiện tại vị trí xuất phát khi bắt đầu đếm ngược, có thể xoay góc nhìn và được di chuyển, nhảy, ngồi hoặc bắn khi hết giờ. Online có **Shooting Error**, vết đạn, đường đạn đối thủ và âm thanh hạ gục. Từ báo cáo Auto-Calibration, bấm **Try in Online 1v1** để dùng sens đề xuất.

Hai máy kết nối ra relay qua WebSocket bảo mật trên TCP 443 để vào phòng và trao đổi thông tin bắt tay. Game thử thiết lập WebRTC DataChannel trực tiếp qua UDP. Khi trận xác nhận đường trực tiếp, hai máy đóng kết nối WebSocket và gửi trạng thái trận, input, gói đo RTT thẳng cho nhau. Nếu đường trực tiếp không thiết lập được, trận chạy qua WebSocket relay. HUD hiển thị **Direct** hoặc **Relay** cùng RTT của đường đang dùng. Nếu đường trực tiếp mất trong lúc đấu, tạo phòng mới để kết nối lại.

Tâm ngắm, ADS và di chuyển của chính bạn phản hồi tại máy bạn. Game mô phỏng di chuyển ngay và hiệu chỉnh theo trạng thái host gửi về. Host chốt thời điểm bắn, đường đạn và sát thương tại 128 Hz với bù trễ từ lịch sử hitbox. RTT vẫn ảnh hưởng đến vị trí đối thủ và thời gian xác nhận phát bắn. Dùng cùng phiên bản game trên hai máy. Mở menu trong trận không dừng trận.

**Ping Fair** bật mặc định và do chủ phòng chọn trước khi tạo phòng. Game đo RTT và thời gian chuyển dữ liệu theo từng chiều trên đường Direct hoặc Relay, rồi chốt độ đệm chung cho trận đó. Hai người có cùng hạn xử lý input và cùng mốc nhận kết quả phát bắn. Trạng thái phát bắn và hồi sinh được gửi ngay trong tick xử lý. Chuyển động đối thủ có thêm hai tick nội suy riêng. Di chuyển và rê tâm của chính mình vẫn phản hồi ngay. HUD hiển thị RTT đo được và `Fair X+Y ms`, trong đó `X` là độ đệm input và `Y` là độ đệm xác nhận. Bỏ chọn **Ping Fair (host)** để thử chế độ host xử lý input ngay ở tick khả dụng.

### Deploy relay Node.js lên Render Free

1. Đẩy repo lên GitHub, gồm `render.yaml` và thư mục `relay/`.
2. Trong [Render Dashboard](https://dashboard.render.com/), chọn **New → Blueprint**, kết nối repo và triển khai service. Blueprint dùng gói **Free**, region **Singapore**, Node.js, thư mục `relay` và health check `/health`. Nếu tạo thủ công bằng **New → Web Service**, chọn cùng region và gói. Đặt **Root Directory** là `relay`, **Build Command** là `npm ci --omit=dev`, **Start Command** là `npm start` và **Health Check Path** là `/health`. [Tài liệu Blueprint](https://render.com/docs/blueprint-spec)
3. Sau khi service báo **Live**, mở `https://<tên-service>.onrender.com/health` và chờ hiện `ok`. Dùng hostname thực trong Dashboard để tạo URL cho game dạng `wss://<tên-service>.onrender.com/v1`. Render cấp HTTPS và chuyển tiếp WebSocket tới tiến trình Node.js. [WebSocket trên Render](https://render.com/docs/websocket)

Render Free có thể ngủ sau 15 phút không nhận lưu lượng. Lần đánh thức có thể mất khoảng một phút. Nếu game báo không kết nối được sau khi dịch vụ nghỉ lâu, mở `/health` trên trình duyệt, chờ `ok` rồi tạo phòng hoặc vào lại. Phòng được lưu trong RAM và sẽ mất nếu Render khởi động lại hoặc deploy phiên bản mới. [Giới hạn Render Free](https://render.com/docs/free)

### Đổi URL và gửi bản Windows cho bạn bè

Đổi URL theo một trong hai cách:

- Trong **Online 1v1**, nhập URL trước khi tạo hoặc vào phòng. Nếu bản build có relay tích hợp, mở **Relay settings** để đổi. Game lưu URL khi thoát.
- Đóng game trước khi sửa `settings.ini` cạnh `ValorantSensFinder.exe`. Có thể sao chép từ `settings.example.ini` rồi sửa hostname. Nếu đã có `settings.ini`, thêm hoặc sửa phần `[online]` hiện có để giữ các thiết lập khác.

```ini
[online]
relay_url=wss://your-relay.onrender.com/v1
```

URL trong `settings.ini` ưu tiên hơn URL tích hợp. Xóa giá trị để dùng lại URL tích hợp. Khi đổi hosting, chỉ cần sửa file này hoặc nhập URL mới trong game. Người chơi chỉ cần app và file cấu hình đã điền sẵn.

### Chạy relay cục bộ

Với Node.js 24, chạy tại thư mục project:

```powershell
cd relay
npm ci --omit=dev
npm start
```

Dùng `ws://127.0.0.1:8765/v1` khi thử trên cùng máy. Bản relay Python cũ vẫn chạy được với `python -m pip install -r relay/requirements.txt` và `python relay/server.py` từ thư mục project.

### Đóng gói bản cho người chơi

Sau khi relay công khai hoạt động, nhúng địa chỉ cố định khi build:

```powershell
cmake -S . -B build "-DVSF_PUBLIC_RELAY_URL=wss://relay.example.com/v1"
cmake --build build --config Release
cmake --install build --config Release --prefix Release
```

Bản đóng gói dùng URL tích hợp khi chưa có địa chỉ trong `settings.ini`. Có thể để trống `-DVSF_PUBLIC_RELAY_URL=` nếu muốn gửi cùng một bản build và quản lý URL hoàn toàn bằng file cấu hình.

## Chỉ số

| Chỉ số | Ý nghĩa |
|---|---|
| Fitts’s Throughput (bits/s) | Kết hợp tốc độ và độ chính xác khi đưa tâm đến mục tiêu. Điểm cao hơn thể hiện khả năng ngắm hiệu quả hơn trong cùng bài. |
| cm/360° | Quãng đường chuột di chuyển để camera quay đủ một vòng. Giá trị thấp hơn tương ứng với độ nhạy cao hơn. |
| Bias Index (−1 đến +1) | Số âm cho thấy xu hướng vẩy hụt, số dương cho thấy xu hướng vẩy quá. Điểm gần 0 thể hiện ít thiên lệch hướng. Thuật toán xét cả đỉnh vẩy trước khi tâm được kéo lại. |

## Điều khiển

| Phím | Tác dụng |
|---|---|
| **F10** | Về cửa sổ thường. Bài đang chạy sẽ tạm dừng. |
| **F11** | Chuyển giữa Borderless và Fullscreen. Bài đang chạy sẽ tạm dừng và hiện con trỏ. |
| **Tab** | Mở menu tạm dừng |
| **Alt+Tab** | Chuyển ứng dụng. Bài đang chạy sẽ tạm dừng khi mất focus. |
| **Chuột trái** | Bắn khi đang trong phòng tập |
| **Esc** | Tạm dừng khi đang chơi, bấm lần nữa để tiếp tục |

Trong menu tạm dừng, chọn **Continue** để tiếp tục đúng chỗ đang chơi, **Main Menu** để kết thúc bài hiện tại, hoặc **Exit Game** để lưu cấu hình và thoát.
