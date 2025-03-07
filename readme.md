# wmata-timetables
Hello :) This is a hackathon project for the [rebble hackathon #002](https://rebble.io/hackathon-002/). The goal of this hackathon project is to create an application that allows users to view the next train times for the Washington Metro Area Transit Authority (WMATA) train systems, by both line and by station.

## Authors
- [Julia Paluch](https://github.com/juliapaluch)
- [Jacob Latonis](https://github.com/latonis)

## Building the application

```bash
rebble build
```

## Emulating the application

```bash
rebble install --emulator aplite
```

## Viewing logs

```bash
rebble logs
```

## Ensure JS works build command
```bash
rebble wipe && rebble clean && rebble build && rebble install --emulator aplite --logs
```