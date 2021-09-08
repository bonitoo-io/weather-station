import React from 'react';
import { Checkbox, FormControlLabel} from '@material-ui/core';
import { ValidatorForm } from 'react-material-ui-form-validator';


import SaveIcon from '@material-ui/icons/Save';

import { RestFormProps, FormActions, FormButton,  } from '../components';
import { UpdateSettings, } from './types';
import { TimePicker, MuiPickersUtilsProvider  } from '@material-ui/pickers'
import { MaterialUiPickersDate  } from '@material-ui/pickers/typings/date'
import DateFnsUtils from '@date-io/date-fns'

type UpdateSettingsFormProps = RestFormProps<UpdateSettings>;

function numberToTime(tm: number) {
  let dt : MaterialUiPickersDate = new Date(0)
  dt.setHours(Math.floor(tm/100))
  dt.setMinutes(Math.floor(tm % 100))
  return dt
}


class UpdateSettingsForm extends React.Component<UpdateSettingsFormProps> {

  changeTime = (valueSetter: (name: string, val: any)=>void ) =>(date: MaterialUiPickersDate) => {
    if(!date) {
      return
    }
    const val = date.getHours()*100+date.getMinutes()
    valueSetter('updateTime', val)
  } 

  render() {
    const { data, handleValueChange, handleDirectValueChange, saveData } = this.props;
    const updateTime = numberToTime(data.updateTime)
    return (
      <ValidatorForm onSubmit={saveData}>
        <MuiPickersUtilsProvider utils={DateFnsUtils}>
          <TimePicker
            label="Update time"
            value={updateTime}
            ampm={false}
            onChange={this.changeTime(handleDirectValueChange)}
          />
        </MuiPickersUtilsProvider>
        <div>
        <FormControlLabel
          value="start"
          control={
            <Checkbox
            value="checkBeta"
            checked={data.checkBeta}
            onChange={handleValueChange("checkBeta")}
          />
          }
          label="Update to beta releases"
          labelPlacement="end"
        />
        </div>
        <div>
        <FormControlLabel
          value="start"
          control={
            <Checkbox
            value="verifyCert"
            checked={data.verifyCert}
            onChange={handleValueChange("verifyCert")}
          />
          }
          label="Verify Github certificate"
          labelPlacement="end"
        />
        </div>
        <FormActions>
          <FormButton startIcon={<SaveIcon />} variant="contained" color="primary" type="submit">
            Save
          </FormButton>
        </FormActions>
      </ValidatorForm>
  );
  }

}

export default UpdateSettingsForm;
