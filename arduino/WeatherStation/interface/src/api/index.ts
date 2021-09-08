export * from './Env'
export * from './Endpoints'

export function formatNumber(num: number, minPlaces = 1) {
    return new Intl.NumberFormat('en-US', { 
        minimumIntegerDigits:  minPlaces
    }).format(num);
  }