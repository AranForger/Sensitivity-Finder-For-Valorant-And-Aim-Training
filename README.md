# Valorant Sensitivity Finder

Ứng dụng luyện aim và tìm độ nhạy chuột cho VALORANT trên Windows.

## Chạy ứng dụng

1. Sao chép thư mục `Release` vào một nơi có quyền ghi trên máy Windows 10/11 x64 có trình điều khiển đồ họa hỗ trợ OpenGL 3.3. Mở `ValorantSensFinder.exe`. Bản đóng gói chạy trực tiếp và không cần Visual C++ Redistributable.
2. Trong **Benchmarks**, chọn **Full** hoặc **Condensed** để luyện từng bài. Trong **Crosshair**, chọn preset hoặc chỉnh tâm ngắm. Ở bài Tracking, đầu bot có đường kính 30 cm và bot chuyển xanh khi tâm ngắm chạm đầu hoặc thân.
3. Để tìm sens, nhập **Mouse DPI**, chọn sens hiện tại hoặc mức gợi ý **280 eDPI**, rồi bấm **Start Auto-Calibration**. Sau khi hoàn tất, **Copy Sens** sao chép sens đề xuất và **Save Results** lưu báo cáo CSV.

Auto-Calibration đếm ngược **3–2–1** trước mỗi vòng. Bật **Blind Sens** để ứng dụng chọn điểm bắt đầu bí mật trong khoảng **245–315 eDPI** và chỉ hiển thị sens cùng điểm số khi có báo cáo cuối.

Ứng dụng đọc `settings.ini` khi mở và lưu DPI, sens luyện tập, thiết lập Auto-Calibration và tâm ngắm khi thoát. File cấu hình, `telemetry/` và `history/` nằm cạnh file `.exe`. Sao chép các file và thư mục này nếu muốn mang theo cấu hình và dữ liệu giữa các máy.

Ứng dụng mở ở **Fullscreen**. Tùy chọn **Low Latency** trong menu tắt VSync và giới hạn công việc dựng hình đang chờ trên GPU trong lúc tập. Tùy chọn mặc định tắt, được lưu trong `settings.ini` và có thể gây xé hình hoặc giảm FPS khi bật.

Trong **Appearance**, bạn có thể đổi màu nền và các bề mặt của phòng, mục tiêu, neo, trạng thái bám trúng và tia lửa. **Audio** cho phép tắt tiếng, chỉnh âm lượng tổng hoặc chỉnh và nghe thử từng âm bắn, bắn trúng và đếm ngược. Màu sắc và âm lượng được lưu trong `settings.ini`.

Trong **Appearance → Graphics**, chọn **Anti-aliasing (MSAA)** ở mức **Off**, **2x**, **4x** (mặc định) hoặc **8x**. Thay đổi áp dụng ngay cho phòng tập 3D. Nếu GPU không hỗ trợ mức đã chọn, ứng dụng hiển thị mức thấp hơn đang dùng.

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
