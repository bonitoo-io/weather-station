const VALID_CHARS = "ADITOFWMC"
export default function isValidScreens(list: string) {
  list = list.toUpperCase()
  for (var i = 0; i < list.length; i++) {
    if (VALID_CHARS.indexOf(list.charAt(i)) === -1) {
      return false
    }
  }
 
  return true
}
