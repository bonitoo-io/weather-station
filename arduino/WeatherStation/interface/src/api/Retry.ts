export const RETRY_EXCEPTION_TYPE = "retry"
export const NUM_POLLS = 20
export const POLLING_FREQUENCY = 500

export function retryError503() {
  return {
    name: RETRY_EXCEPTION_TYPE,
    message: "Data not ready, will retry"
  };
}

export function retryErrorPolling() {
  return {
    name: RETRY_EXCEPTION_TYPE,
    message: "Validation not ready, will retry in " + POLLING_FREQUENCY + "ms."
  };
}
