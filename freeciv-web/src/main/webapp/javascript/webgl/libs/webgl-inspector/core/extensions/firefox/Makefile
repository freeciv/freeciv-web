PROFILE_ARG=''

ifdef PROFILE
	PROFILE_ARG = -p $(PROFILE)
endif

run: clean
	# I usually run with: `PROFILE=/path/to/dev/profile make run`.
	cd ../.. && ./buildextensions.sh # will run `make build`
	cd addon-sdk && . bin/activate && cd .. && cfx run $(PROFILE_ARG)

build:
	cd addon-sdk && . bin/activate && cd .. && cfx xpi

clean:
	rm -rf webglinspector.xpi data/assets data/gli.all.css data/gli.all.js

