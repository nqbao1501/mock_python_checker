# BÁO CÁO MOCK PROJECT  
# Python Version Checker trên OpenWrt sử dụng Docker


# 1. Giới thiệu

Project này mô phỏng quá trình phát triển một ứng dụng userspace cho hệ thống OpenWrt chạy trên Raspberry Pi 4B.  
Ứng dụng được viết bằng ngôn ngữ C và có chức năng kiểm tra phiên bản Python 3.9 được cài đặt trong hệ thống.

Toàn bộ quá trình build và kiểm thử được thực hiện trong môi trường Docker nhằm mô phỏng môi trường embedded Linux tối giản giống OpenWrt.

Ngoài việc phát triển ứng dụng C, project còn sử dụng:
- Docker
- Git
- Makefile
- OpenWrt package (`.ipk`)

---

# 2. Cấu trúc Project

```text
.
├── builder
│   └── Dockerfile
├── runtime
│   └── Dockerfile
│   └── openwrt-21.02.7-x86-64-rootfs.tar.gz
│   └── openwrt-23.05.0-x86-64-rootfs.tar.gz
├── src/
│   └── python-check
│       └── src
│           └── python-check.c
│       └── Makefile
├── bin/
│   └── python-check_1.0-1_x86_64.ipk
└── Makefile
```
---

# 3. Root Makefile

Project sử dụng một Makefile ở thư mục gốc để tự động hóa toàn bộ quá trình:
- build Docker image
- build package `.ipk`
- chạy OpenWrt runtime
- dọn dẹp file build

Makefile giúp giảm thao tác thủ công và tạo workflow thống nhất cho quá trình build/test.

---

## Nội dung Root Makefile

```make
.PHONY: docker-builder docker-runtime builder-shell runtime-shell package clean

BUILDER_IMAGE=openwrt-builder
RUNTIME_IMAGE=openwrt-runtime

docker-builder:
	docker build -t $(BUILDER_IMAGE) ./builder

docker-runtime:
	docker build -t $(RUNTIME_IMAGE) ./runtime

builder-shell:
	docker run -it --rm \
		-v $(PWD):/workspace \
		$(BUILDER_IMAGE)

package:
	docker run -it --rm \
		-v $(PWD):/workspace \
		$(BUILDER_IMAGE) \
		/bin/bash -c "\
		cp -r /workspace/package/python-check /opt/openwrt-sdk/package/ && \
		cd /opt/openwrt-sdk && \
		make package/python-check/compile V=s && \
		mkdir -p /workspace/bin && \
		cp /opt/openwrt-sdk/bin/packages/x86_64/base/*.ipk /workspace/bin/"

runtime-shell:
	docker run -it --rm \
		--name openwrt-test \
		-v $(PWD)/bin:/root/packages \
		$(RUNTIME_IMAGE) /bin/sh

clean:
	rm -rf bin/
```

##Chi tiết các lệnh Makefile
| Lệnh (`make <target>`) | Chức năng chi tiết |
| :--- | :--- |
| **`make docker-builder`** | Khởi tạo Docker Image chứa bộ OpenWrt SDK Toolchain, chuẩn bị sẵn sàng cho việc biên dịch chéo phần mềm từ mã nguồn C sang gói ứng dụng nhúng. |
| **`make docker-runtime`** | Khởi tạo Docker Image môi trường đích (target environment) dựa trên vi kiến trúc lõi hệ điều hành OpenWrt sạch để chạy nghiệm thu. |
| **`make builder-shell`** | Khởi chạy container của Builder dưới dạng Terminal tương tác (ash/bash) dùng để debug cấu hình hoặc kiểm tra thủ công bên trong SDK. |
| **`make package`** | Lõi tự động hóa: Đồng bộ mã nguồn ứng dụng python-check vào cấu trúc cây thư mục SDK, kích hoạt tiến trình biên dịch chéo sang định dạng gói cài đặt .ipk, sau đó trích xuất thành phẩm ra thư mục bin/ trên máy host. |
| **`make runtime-shell`** | Khởi chạy môi trường OpenWrt Runtime đích, tự động gắn kết (mount) thư mục bin/ chứa file .ipk vào phân vùng /root/packages để tiến hành cài đặt kiểm thử. |
| **`make clean`** | Loại bỏ thư mục bin/ và toàn bộ các file trung gian sinh ra sau quá trình đóng gói, trả lại trạng thái sạch (clean state) cho Workspace dự án. |

---

# 5. Chức năng chương trình

Ứng dụng thực hiện các bước sau:

1. Kiểm tra sự tồn tại của `python3.9`
2. Nếu tồn tại:

   * chạy lệnh `python3.9 --version`
   * đọc output
   * in ra terminal
   * ghi log vào `/tmp/python_ver.log`
3. Nếu không tồn tại:

   * hiển thị lỗi
   * trả về exit code khác 0

---

# 6. Các trường hợp xử lý

| Trường hợp                  | Kết quả                           |
| --------------------------- | --------------------------------- |
| Không có Python 3.9         | Error: Python 3.9 not found       |
| Có Python nhưng sai version | Python exists but NOT version 3.9 |
| Có đúng Python 3.9          | Python 3.9 OK                     |

---

# 7. Source Code chính

Ứng dụng sử dụng:

* `system()` để kiểm tra command tồn tại
* `popen()` để đọc output từ command

Ví dụ:

```c
system("which python3.9 > /dev/null 2>&1");
```

```c
popen("python3.9 --version 2>&1", "r");
```

---

# 8. Docker Environment

## 8.1 Mục đích

Docker được sử dụng để:

* mô phỏng môi trường embedded Linux
* build và test package
* tránh phụ thuộc vào host machine

---

## 8.2 Build Docker Image

```bash
docker build -t openwrt-runtime .
```

---

## 8.3 Chạy Container

```bash
docker run -it --rm \
    --name openwrt-test \
    -v $(pwd)/bin:/root/packages \
    openwrt-runtime /bin/sh
```

---

# 9. Cài đặt Python trên OpenWrt

Trong container OpenWrt:

```sh
mkdir -p /var/lock
opkg update
opkg install python3-light
```

Kiểm tra:

```sh
python3 --version
```

---

# 10. Tạo Wrapper python3.9

OpenWrt mặc định chỉ tạo command `python3`, do đó cần tạo wrapper:

```sh
cat > /usr/bin/python3.9 << 'EOF'
#!/bin/sh
exec /usr/bin/python3 "$@"
EOF

chmod +x /usr/bin/python3.9
```

---

# 11. Makefile Automation

Project sử dụng Makefile để tự động hóa build và package.

## Các target chính

| Command            | Chức năng            |
| ------------------ | -------------------- |
| make               | Compile chương trình |
| make run           | Chạy chương trình    |
| make package       | Build package `.ipk` |
| make clean         | Xóa file build       |
| make runtime-shell | Mở OpenWrt container |

---

# 12. OpenWrt Packaging

Ứng dụng được đóng gói thành package `.ipk`.

## Build package

```bash
make package
```

Output:

```text
python-check_1.0-1_x86_64.ipk
```

---

## Install package

```sh
opkg install /root/packages/python-check_1.0-1_x86_64.ipk
```

---

# 13. Kết quả chạy chương trình

## Trường hợp đúng Python 3.9

```text
Detected: Python 3.9.16
CASE 3: Python 3.9 OK
```

---

## File log

```text
/tmp/python_ver.log
```

Ví dụ nội dung:

```text
CASE 3: Python 3.9 OK (Python 3.9.16)
```

---

# 14. Git Version Control

Project sử dụng Git để quản lý source code.

## Branch sử dụng

```text
feature/python-version-check
```

## Tag release

```text
v1.0-python-check
```

---

# 15. Khó khăn gặp phải

Trong quá trình thực hiện project, một số vấn đề đã xảy ra:

* OpenWrt package version mismatch
* Python package khác version OpenWrt runtime
* Symbolic link loop khi tạo `python3.9`
* Runtime dependency mismatch giữa các package

Ví dụ:

* build package trên OpenWrt 23.05
* chạy trên OpenWrt 21.02

Điều này gây lỗi ABI và runtime.

---

# 16. Bài học rút ra

Qua project này, đã học được:

* Cách sử dụng Docker để mô phỏng môi trường embedded Linux
* Cách build package `.ipk`
* Cách sử dụng `opkg`
* Cách làm việc với OpenWrt runtime
* Sử dụng Git branch và tag
* Tự động hóa build bằng Makefile
* Xử lý process trong C bằng `system()` và `popen()`

---

# 17. Kết luận

Project đã hoàn thành các yêu cầu chính:

* Build và test trong Docker
* Viết ứng dụng C kiểm tra Python version
* Build package `.ipk`
* Tích hợp OpenWrt runtime
* Sử dụng Git để quản lý source code
* Tự động hóa bằng Makefile

Project có thể mở rộng thêm trong tương lai:

* Deploy lên Raspberry Pi 4B thật
* Build full OpenWrt image
* Tích hợp CI/CD pipeline
* Auto package signing

```
```
