Since TCP cubic and BBR needed custom patches to ns3, I just commited the updated ns3 code
in this repo.

Pull the repo, and run
```
./waf configure
./waf
```

Our code is present in scratch/tcp-variant-test

To run the code,

```
cd scratch/tcp-variant-test
mv .env.sample .env
```

Edit .env and set ROOT_DIR to the current folder

```
source ./.env
./run.sh
```

Make sure the waf executable is added to your $PATH
