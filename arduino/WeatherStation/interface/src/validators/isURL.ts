export default function isURL(urlStr: string) {
  try {
    const url = new URL(urlStr);
    return url.protocol.toLowerCase().startsWith('http')
  } catch (error) {
    //error; // => TypeError, "Failed to construct URL: Invalid URL"
  }
  return false
}
