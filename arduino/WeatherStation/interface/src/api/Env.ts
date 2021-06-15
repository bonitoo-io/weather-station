export const PROJECT_NAME = process.env.REACT_APP_PROJECT_NAME!;
export const PROJECT_PATH = process.env.REACT_APP_PROJECT_PATH!;

export const ENDPOINT_ROOT = calculateEndpointRoot("/api/");

function calculateEndpointRoot(endpointPath: string) {
    const httpRoot = process.env.REACT_APP_HTTP_ROOT;
    if (httpRoot) {
        return httpRoot + endpointPath;
    }
    const location = window.location;
    return location.protocol + "//" + location.host + endpointPath;
}
