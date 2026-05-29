# BÁO CÁO MOCK PROJECT  
# Python Version Checker trên OpenWrt sử dụng Docker
Link github: 

# 1. Giới thiệu

Project này mô phỏng quá trình phát triển một ứng dụng userspace cho hệ thống OpenWrt chạy trên Raspberry Pi 4B. Tuy nhiên, ở đây ta test luôn trên OpenWrt chip x86.

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
| **`make package`** | Đồng bộ mã nguồn ứng dụng python-check vào cấu trúc cây thư mục SDK, kích hoạt tiến trình biên dịch chéo sang định dạng gói cài đặt .ipk, sau đó trích xuất thành phẩm ra thư mục bin/ trên máy host. |
| **`make runtime-shell`** | Khởi chạy môi trường OpenWrt Runtime đích, tự động gắn kết (mount) thư mục bin/ chứa file .ipk vào phân vùng /root/packages để tiến hành cài đặt kiểm thử. |
| **`make clean`** | Loại bỏ thư mục bin/ và toàn bộ các file trung gian sinh ra sau quá trình đóng gói, trả lại trạng thái sạch (clean state) cho Workspace dự án. |

---

# 4. Chi tiết môi trường Docker (Builder & Runtime)

Dự án áp dụng mô hình thiết kế **Multi-stage Environment**, tách biệt hoàn toàn giữa không gian biên dịch (Compile-time) và không gian chạy thực tế (Run-time). Điều này giúp tối ưu kích thước ảnh độc lập, giữ môi trường thử nghiệm sạch sẽ và tránh việc lây nhiễm các công cụ rác vào hệ điều hành đích.

## 4.1. Môi trường Biên dịch (Builder)


Thư mục `builder/` đảm nhận nhiệm vụ tạo ra một Docker Image chứa toàn bộ các công cụ nền tảng hỗ trợ **Biên dịch chéo (Cross-compilation)**. 

**Hệ điều hành nền:** Sử dụng `ubuntu:22.04`

**Các gói phụ thuộc bắt buộc (Dependencies):** 
* **`build-essential`**: Cung cấp bộ công cụ biên dịch tiêu chuẩn cho Linux (bao gồm `gcc`, `g++`, `libc-dev`), đóng vai trò làm trình biên dịch nền (Host compiler) để build các công cụ nội bộ của SDK trước khi dịch mã nguồn target.
* **`clang`**: Trình biên dịch C/C++ dựa trên hạ tầng LLVM, bổ trợ cho hệ thống trong các tác vụ kiểm tra cú pháp, tối ưu hóa mã nguồn hoặc hỗ trợ biên dịch các gói phần mềm yêu cầu khắt khe về toolchain hiện đại.
* **`git`**: Công cụ quản lý mã nguồn phân tán, được SDK sử dụng ngầm để kéo (fetch) mã nguồn của các gói phụ thuộc (package feeds) từ các kho lưu trữ trực tuyến về máy ảo.
* **`wget`**: Tiện ích mạng cho phép tải các tệp tin qua giao thức HTTP/HTTPS, phục vụ việc download mã nguồn các gói phần mềm mở rộng hoặc các bản vá (patches) trong quá trình dựng package.
* **`file`**: Lệnh hệ thống giúp nhận biết và phân loại định dạng tệp tin (ELF executable, script, text, v.v.). SDK dùng công cụ này để kiểm tra tính hợp lệ của các file thực thi sau khi biên dịch chéo.
* **`python3`** & **`python3-distutils`**: Bộ đôi tối quan trọng. Bản thân lõi OpenWrt sử dụng rất nhiều kịch bản hệ thống viết bằng Python 3 để xử lý siêu dữ liệu (metadata) của các package. Gói `distutils` đi kèm là bắt buộc để các script này có thể phân tích cấu hình phần cứng, quản lý module cài đặt nâng cao mà không bị gãy tiến trình (fatal error).
* **`libncurses5-dev`**: Thư viện cung cấp API phát triển giao diện đồ họa Terminal trực quan. Gói này trực tiếp phục vụ cho menu cấu hình tương tác trực quan của OpenWrt (như khi thực hiện lệnh `make menuconfig`).
* **`gawk`**: Phiên bản cải tiến của ngôn ngữ xử lý văn bản AWK, được các kịch bản của OpenWrt dùng liên tục để bóc tách, lọc chuỗi dữ liệu cấu hình từ các file cấu hình hệ thống.
* **`unzip`**, **`tar`**, **`gzip`**: Bộ ba công cụ nén và giải nén cơ bản, chịu trách nhiệm giải nén mã nguồn các thư viện tải về dạng `.tar.gz`, `.zip` và chuẩn bị môi trường cho cây thư mục biên dịch.
* **`zlib1g-dev`**: Thư viện cung cấp các hàm nén dữ liệu tĩnh, cần thiết cho tiến trình liên kết (Linking phase) của các gói phần mềm cần tối ưu dung lượng trên bộ nhớ nhúng.
* **`flex`** & **`bison`**: Bộ đôi công cụ sinh trình phân tích cú pháp (Lexical analyzer và Parser generator). Chúng giúp SDK đọc hiểu, biên dịch các tệp cấu hình hệ thống phác thảo bằng ngôn ngữ đặc thù của OpenWrt.
* **`gettext`**: Hệ thống hỗ trợ quốc tế hóa và địa phương hóa ngôn ngữ (I18n), giúp biên dịch các file thông báo lỗi hoặc nhãn hiển thị trong hệ thống.
* **`bash`**: Shell dòng lệnh tiêu chuẩn, cung cấp môi trường thực thi mạnh mẽ cho các kịch bản cài đặt tự động phức tạp vốn không chạy được trên các shell tối giản như `sh`.
* **`rsync`**: Công cụ đồng bộ hóa dữ liệu tốc độ cao, hỗ trợ SDK sao chép nhanh các tệp tin cấu trúc lớn giữa các phân vùng xây dựng mà vẫn giữ nguyên vẹn thuộc tính file và quyền truy cập (`chmod`).
* **`make`**: Trình quản lý và tự động hóa tiến trình biên dịch dựa trên kịch bản `Makefile`, chịu trách nhiệm đọc chuỗi lệnh phối hợp từ file cấu hình của package để kích hoạt quá trình ra đời của tệp tin `.ipk`.

**Lõi Toolchain (OpenWrt SDK):** Tự động tải và cấu hình bộ **OpenWrt Software Development Kit (SDK)** tương ứng từ máy chủ chính thức của OpenWrt dựa theo trạng thái phiên bản đang kiểm thử (`21.02` hoặc `23.05`). SDK này chứa các trình biên dịch chéo đặc thù (như `x86_64-openwrt-linux-musl-gcc`) giúp ép mã nguồn C biên dịch chính xác ra kiến trúc phần cứng đích thay vì sử dụng trình biên dịch thông thường của máy Host.


## 4.2. Môi trường Kiểm thử (Runtime)

Thư mục `runtime/` chịu trách nhiệm khởi dựng một mô phỏng hệ điều hành nhúng OpenWrt nguyên bản.

* **Kiến trúc tối giản (`FROM scratch`):** Dockerfile được xây dựng từ một layer trống rỗng (`scratch`), giúp loại bỏ hoàn toàn các layer trung gian của hệ điều hành máy Host, đảm bảo môi trường tiệm cận nhất với một thiết bị Router vật lý thực tế.
* **Tích hợp Hệ thống tệp gốc (Rootfs):** Sử dụng lệnh `ADD` để trực tiếp giải nén tệp tin `openwrt-*-rootfs.tar.gz` vào thư mục gốc `/`. Hệ thống tệp này chứa đầy đủ các phân vùng tiêu chuẩn của OpenWrt, bao gồm cả trình quản lý gói `opkg`, hệ thống shell `busybox` cấu hình thấp.
* **Mục tiêu kiểm thử:** Image này đóng vai trò làm môi trường "sandbox" sạch hoàn toàn, dùng để cài đặt thử nghiệm tệp tin `.ipk` thành phẩm sinh ra từ Builder, nhằm cô lập hành vi và kiểm tra chính xác các logic tương tác hệ thống của ứng dụng `python-check`.

---

# 5. Chi tiết Ứng dụng Biên dịch và Đóng gói Package (.ipk)

Để triển khai một phần mềm lên hệ điều hành nhúng OpenWrt, mã nguồn không thể chạy trực tiếp dưới dạng file thực thi (binary) thông thường của máy Host, mà bắt buộc phải đóng gói thành định dạng `.ipk`.

---

## 5.1. Khái niệm Gói ứng dụng .ipk là gì?

Gói `.ipk` (Itsy Package Management) là định dạng đóng gói phần mềm tiêu chuẩn được sử dụng trong các hệ điều hành Linux nhúng như OpenWrt.

* **Bản chất cấu trúc:** Tương tự như gói `.deb` của Debian/Ubuntu, file `.ipk` thực chất là một tệp nén chứa ba thành phần lõi:
  1. **Metadata:** Chứa thông tin về gói (Tên, phiên bản, kiến trúc phần cứng phù hợp, các gói phụ thuộc bắt buộc).
  2. **Control Scripts:** Các kịch bản chạy ngầm khi cài đặt (nhiệm vụ chạy trước/sau khi cài đặt hoặc gỡ bỏ gói).
  3. **Data Files:** Chứa file thực thi đã biên dịch chéo (binary) và cấu hình thư mục sẽ được giải nén trực tiếp vào hệ thống tệp gốc (rootfs) của thiết bị.
* **Trình quản lý:** OpenWrt sử dụng công cụ `opkg` để đọc, cài đặt, cập nhật hoặc gỡ bỏ các file `.ipk` này trên thiết bị mục tiêu.

---

## 5.2. Quy trình Sinh ra Gói .ipk thông qua OpenWrt SDK

Tiến trình tạo ra một file `.ipk` từ mã nguồn C trải qua các giai đoạn tự động ngầm bên trong OpenWrt SDK:

* **Bước 1: Khởi tạo không gian xây dựng:** SDK đọc thông tin từ file `Makefile` của package, tạo ra một thư mục tạm mang tên `$(PKG_BUILD_DIR)`.
* **Bước 2: Kích hoạt Biên dịch chéo (Cross-compile):** SDK truyền trình biên dịch đích đặc thù `$(TARGET_CC)` đi kèm các cờ tối ưu hóa `$(TARGET_CFLAGS)` để dịch file mã nguồn `python-check.c` thành tệp thực thi chạy trên kiến trúc x86_64.
* **Bước 3: Định tuyến cây thư mục ảo (Staging):** SDK tạo ra một cấu trúc thư mục ảo mô phỏng hệ điều hành nhúng, sao chép file thực thi vào đúng phân vùng mong muốn (ví dụ: `/usr/bin/`).
* **Bước 4: Đóng gói thành phẩm:** Trình đóng gói nén toàn bộ các tệp tin cùng thông tin Metadata lại để xuất ra file `.ipk` cuối cùng nằm trong thư mục `bin/packages/`.

---
## 5.3. Makefile của Package
Makefile này không hoạt động theo cơ chế biên dịch thông thường mà sử dụng hệ thống macro mở rộng đặc thù của OpenWrt Buildroot nhằm khai báo siêu dữ liệu và chỉ định các bước đóng gói phần mềm.

```make
include $(TOPDIR)/rules.mk

# Khai báo thông tin nền tảng của Package
PKG_NAME:=python-check
PKG_VERSION:=1.0
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

# Định nghĩa siêu dữ liệu hiển thị trong trình quản lý opkg hoặc menuconfig
define Package/python-check
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=Python Version Checker
endef

# Định nghĩa chỉ thị biên dịch chéo mã nguồn C
define Build/Compile
	$(TARGET_CC) $(TARGET_CFLAGS) \
		-o $(PKG_BUILD_DIR)/python-check \
		./src/python-check.c
endef

# Định nghĩa quy tắc cài đặt cấu trúc tệp tin lên hệ thống đích
define Package/python-check/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) \
		$(PKG_BUILD_DIR)/python-check  \
		$(1)/usr/bin/
endef

$(eval $(call BuildPackage,python-check))
```

# 6. Sử dụng chương trình 

## 6.1. Cấu hình git
## 6.1. Cấu hình Git và Quản lý Nhánh (Git Integration)

Để đáp ứng trọn vẹn cả yêu cầu đặc tả của đề bài lẫn bài toán kiểm thử thực tế trên nhiều nền tảng, kiến trúc Git của dự án được phân tách rõ ràng thành **03 nhánh độc lập** với hai vai trò khác nhau (02 nhánh chức năng môi trường và 01 nhánh tính năng nộp bài):

* **Nhánh Chức năng Môi trường (`openwrt-21.02` và `openwrt-23.05`):** Hai nhánh này đóng vai trò cô lập cấu hình hệ thống tệp gốc (`rootfs`) và các bộ SDK tương ứng của từng phiên bản OpenWrt. Cơ chế này giúp chuyển đổi nhanh môi trường kiểm thử (Context Switch) ngay trên máy Host mà không cần duy trì nhiều thư mục dự án cồng kềnh.
* **Nhánh Tính năng Yêu cầu (`feature/python-version-check`):** Nhánh bắt buộc theo đặc tả của đề bài. Nhánh này được rẽ nhánh từ môi trường phát triển, tổng hợp mã nguồn ứng dụng `check_python.c` hoàn chỉnh cùng bộ Dockerfile ổn định nhất để làm phân vùng nghiệm thu cuối cùng.

```text
                      ┌──> openwrt-21.02 (Môi trường test Python 3.9)
                      │
─── [Nhánh gốc] ──────┼──> openwrt-23.05 (Môi trường test Python 3.11)
                      │
                      └──> feature/python-version-check (Nhánh nộp bài chính thức)
                                     ▲
                                     └─ [Gắn Tag Release: v1.0-python-check]


```
* **Kiểm tra danh sách nhánh hiện có:**
```bash
lilac@lilac-Inspiron-5557:~/mock_python_checker$ git branch
* openwrt-21.02
  openwrt-23.05
  feature/python-version-check
## 6.2. Quy trình build package
Quy trình biên dịch chéo tự động được thực hiện tuần tự thông qua công cụ Root Makefile nhằm giảm thiểu tối đa các thao tác gõ lệnh thủ công:

**Bước 1**: Khởi tạo các môi trường Docker
Xây dựng Image làm Host biên dịch chéo (openwrt-builder) và Image làm OS mục tiêu sạch (openwrt-runtime):
```bash
make docker-builder
make docker-runtime
```
**Bước 2**: Thực hiện Biên dịch chéo và tạo ipk
Kích hoạt container Builder để nạp mã nguồn vào SDK, ép cấu trúc compile và trích xuất file đóng gói ra ngoài máy Host:
```bash
make package
```

**Bước 3**: Truy cập môi trường Runtime để tiến hành nghiệm thu
Khởi chạy container runtime mô phỏng OpenWrt, tự động gắn kết (mount) thư mục bin/ chứa file cài đặt vào phân vùng /root/packages bên trong Container:
```bash
make runtime-shell
```

## 6.3. Kết quả thử nghiệm
Sau khi truy cập vào shell của container runtime, ta tiến hành cài đặt gói bằng trình opkg 
```bash
mkdir -p /var/lock
cd /root/packages
opkg install python-check_1.0-1_x86_64.ipk
```

Bây giờ, do openwrt rootfs chưa có python, nên khi chay python-test sẽ không thấy python trong hệ điều hành.
<img width="654" height="187" alt="image" src="https://github.com/user-attachments/assets/a297bf2f-d0b9-47da-921b-fc9e778a234d" />

Ta thực hiện tải python vào trong container. Trong OpenWRT 21.02.7, python là python 3.9.16 
```bash
opkg update && opkg install python3
python-check
```
<img width="870" height="544" alt="image" src="https://github.com/user-attachments/assets/75a0c3cf-3be9-4c6d-b928-2e17e9d29d48" />


Ta chuyển sang OpenWrt 23.05.0 có python 3.11 để test.
```bash
git switch openwrt-23.05
```
Rồi thực hiện các bước tương tự như trên
<img width="870" height="544" alt="image" src="https://github.com/user-attachments/assets/b91dea2d-924d-4f40-b93f-9c22646dbf60" />

Và kết quả log cũng được lưu vào log file
<img width="870" height="102" alt="image" src="https://github.com/user-attachments/assets/d755a664-9424-4bf2-96f6-91f286c1a72f" />


