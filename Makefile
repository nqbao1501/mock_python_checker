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