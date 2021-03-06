import React from 'react';
import { withSnackbar, WithSnackbarProps } from 'notistack';
import { retryError503, RETRY_EXCEPTION_TYPE } from '../api';


export interface RestControllerProps<D> extends WithSnackbarProps {
  handleValueChange: (name: keyof D) => (event: React.ChangeEvent<HTMLInputElement>) => void;
  handleDirectValueChange: (name: string, val: any) => void

  setData: (data: D, callback?: () => void) => void;
  saveData: () => void;
  loadData: () => void;

  data?: D;
  loading: boolean;
  errorMessage?: string;
}

export const extractEventValue = (event: React.ChangeEvent<HTMLInputElement>) => {
  switch (event.target.type) {
    case "number":
      return event.target.valueAsNumber;
    case "checkbox":
      return event.target.checked;
    case "time":
      const time = event.target.valueAsDate
      if(time) {
        return time?.getHours()*100+time?.getMinutes()
      } else {
        return event.target.value
      }
    default:
      return event.target.value
  }
}

interface RestControllerState<D> {
  data?: D;
  loading: boolean;
  errorMessage?: string;
}



export function restController<D, P extends RestControllerProps<D>>(endpointUrl: string, RestController: React.ComponentType<P & RestControllerProps<D>>) {
  return withSnackbar(
    class extends React.Component<Omit<P, keyof RestControllerProps<D>> & WithSnackbarProps, RestControllerState<D>> {

      state: RestControllerState<D> = {
        data: undefined,
        loading: false,
        errorMessage: undefined
      };

      attempts: number  = 0

      setData = (data: D, callback?: () => void) => {
        this.setState({
          data,
          loading: false,
          errorMessage: undefined
        }, callback);
      }

     
      loadData = () => {
        this.attempts = 0
        this.doLoadData()
      }

      doLoadData = () => {
        ++this.attempts
        this.setState({
          data: undefined,
          loading: true,
          errorMessage: undefined
        });
        fetch(endpointUrl).then(response => {
          if (response.status === 200) {
            return response.json();
          } else if (response.status === 503 || response.status ===  429) {
            const retryAfter = response.headers.get("Retry-After")
            let timeout = 1
            if (retryAfter) {
              timeout = parseInt(retryAfter, 10);
            }
            setTimeout(this.loadData, timeout*1000)
            throw retryError503()
          }
          throw Error("Invalid status code: " + response.status);
        }).then(json => {
          this.setState({ data: json, loading: false })
        }).catch(error => {
          if(error.name !== RETRY_EXCEPTION_TYPE) {
            if(this.attempts < 3) {
              setTimeout(this.doLoadData, 5*1000)
            } else {
              const errorMessage = error.message || "Unknown error";
              this.props.enqueueSnackbar("Problem fetching: " + errorMessage, { variant: 'error' });
              this.setState({ data: undefined, loading: false, errorMessage });
            }
          }
        });
      }

      saveData = () => {
        this.attempts = 0
        this.doSaveData()
      }

      doSaveData = () => {
        ++this.attempts
        this.setState({ loading: true });
        fetch(endpointUrl, {
          method: 'POST',
          body: JSON.stringify(this.state.data),
          headers: {
            'Content-Type': 'application/json'
          }
        }).then(response => {
          if (response.status === 200) {
            return response.json();
          } else if (response.status === 503 || response.status ===  429) {
            const retryAfter = response.headers.get("Retry-After")
            let timeout = 1
            if (retryAfter) {
              timeout = parseInt(retryAfter, 10);
            }
            setTimeout(this.saveData, timeout*1000)
            throw retryError503()
          }
          throw Error("Invalid status code: " + response.status);
        }).then(json => {
          this.props.enqueueSnackbar("Update successful.", { variant: 'success' });
          this.setState({ data: json, loading: false });
        }).catch(error => {
          if(error.name !== RETRY_EXCEPTION_TYPE) {
            if(this.attempts < 3) {
              setTimeout(this.doSaveData, 5*1000)
            } else {
              const errorMessage = error.message || "Unknown error";
              this.props.enqueueSnackbar("Problem updating: " + errorMessage, { variant: 'error' });
              this.setState({ data: undefined, loading: false, errorMessage });
            }
          }
        });
      }

      handleValueChange = (name: keyof D) => (event: React.ChangeEvent<HTMLInputElement>) => {
        const val =  extractEventValue(event)
        this.handleDirectValueChange(name as string, val)
      }

      handleDirectValueChange = (name: string, val: any) => {
        const data = { ...this.state.data!, [name]: val};
        this.setState({ data });
      }

      render() {
        return <RestController
          {...this.state}
          {...this.props as P}
          handleValueChange={this.handleValueChange}
          handleDirectValueChange={this.handleDirectValueChange}
          setData={this.setData}
          saveData={this.saveData}
          loadData={this.loadData}
        />;
      }

    });
}
