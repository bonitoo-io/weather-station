import { MaterialUiPickersDate } from '@material-ui/pickers/typings/date';

export * from './Env'
export * from './Endpoints'
export * from './Retry'

export function formatNumber(num: number, minPlaces = 1) {
    return new Intl.NumberFormat('en-US', { 
        minimumIntegerDigits:  minPlaces
    }).format(num);
  }

  export function numberToTime(tm: number) {
    let dt : MaterialUiPickersDate = new Date(0)
    dt.setHours(Math.floor(tm/100))
    dt.setMinutes(Math.floor(tm % 100))
    return dt
  }