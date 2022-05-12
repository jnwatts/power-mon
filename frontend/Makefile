HTTP_ROOT=/mnt/http/sroz.net/root/power

build:
	rm -Rf dist
	mkdir -p dist/api/
	yarn parcel build --public-url .
	rsync -r src/api/ dist/api/

deploy:
	rsync -r dist/ $(HTTP_ROOT)/ --delete

clean:
	rm -Rf dist