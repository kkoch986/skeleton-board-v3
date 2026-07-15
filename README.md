

Things to test:

- [x] status LED
- [ ] audio detection
  - [ ] can read the envelopes
  - [x] can read the presence pin
- [ ] DMX read functionality
- [ ] driving servos
- [ ] reading the address switches
- [ ] test expansion pins
- [ ] test power isolation for servo banks
- [ ] test i2c control of the eyes
- [ ] test wifi set up and configuration

potential upgrades for next version:

- one button that we can read to trigger the device to connect to wifi for configuration
- R and G pins seem to be reversed (R is io10, G is io9), not really a functional issue just a correction to the schematic
- envelopes seem noisy
    detect=1 envelope=1929 offset=1919 amp=10 brightness=1
    detect=1 envelope=1927 offset=1919 amp=8 brightness=1
    detect=1 envelope=1928 offset=1919 amp=9 brightness=1
    detect=1 envelope=1917 offset=1919 amp=2 brightness=0
    detect=1 envelope=1927 offset=1919 amp=8 brightness=1
    detect=1 envelope=1932 offset=1919 amp=13 brightness=1
    detect=1 envelope=1928 offset=1919 amp=9 brightness=1
    detect=1 envelope=1925 offset=1919 amp=6 brightness=0
    detect=1 envelope=1929 offset=1919 amp=10 brightness=1
    detect=1 envelope=1925 offset=1919 amp=6 brightness=0
    detect=1 envelope=1919 offset=1919 amp=0 brightness=0
    detect=1 envelope=1917 offset=1919 amp=2 brightness=0
    detect=1 envelope=1925 offset=1919 amp=6 brightness=0
    detect=1 envelope=1926 offset=1919 amp=7 brightness=0
    detect=1 envelope=1929 offset=1919 amp=10 brightness=1
    detect=1 envelope=1917 offset=1919 amp=2 brightness=0
    detect=1 envelope=1919 offset=1919 amp=0 brightness=0
    detect=1 envelope=1925 offset=1919 amp=6 brightness=0
    detect=1 envelope=1915 offset=1919 amp=4 brightness=0
    detect=1 envelope=1930 offset=1919 amp=11 brightness=1
    detect=1 envelope=1917 offset=1919 amp=2 brightness=0
    detect=1 envelope=1928 offset=1919 amp=9 brightness=1
    detect=1 envelope=1925 offset=1919 amp=6 brightness=0
    detect=1 envelope=1917 offset=1919 amp=2 brightness=0
    detect=1 envelope=1927 offset=1919 amp=8 brightness=1
    detect=1 envelope=1918 offset=1919 amp=1 brightness=0
