import { readFile } from "fs"

// @const
const CPU_TEMPERATURE_TEMP = "/sys/class/thermal/thermal_zone0/temp"

// 获取CPU温度
export function Temperature (): Promise<number> {
    return new Promise((resolve, reject) => {
        readFile(CPU_TEMPERATURE_TEMP, function (err, buf) {
            err ? reject(err) : resolve(Number(buf.toString("utf8")) / 1000)
        })
    })
}
