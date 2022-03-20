HTTP_ROOT=/mnt/http/sroz.net/root/power

build:
	rm -Rf dist
	mkdir -p dist/api/
	yarn parcel build --public-url .
	rsync -rv src/api/ dist/api/

deploy:
	rsync -rv dist/ $(HTTP_ROOT)/ --delete

clean:
	rm -Rf dist