import React, { Component, RefObject } from 'react';
import { TextValidator, ValidatorForm } from 'react-material-ui-form-validator';

import { isValidScreens } from '../validators';
import SaveIcon from '@material-ui/icons/Save';

import { RestFormProps, FormActions, FormButton } from '../components';

import { DisplaySettings } from './types';
import { Theme, createStyles, withStyles, WithStyles } from '@material-ui/core/styles';


const styles = (theme: Theme) => createStyles({ 
   connectingSettings: {
    margin: theme.spacing(0.5),
  },
  connectingSettingsDetails: {
    margin: theme.spacing(4),
    textAlign: "center"
  },
  connectingProgress: {
    margin: theme.spacing(4),
    textAlign: "center"
  }
});

type DisplaySettingsFormProps = RestFormProps<DisplaySettings> & WithStyles<typeof styles>;


class DisplaySettingsForm extends Component<DisplaySettingsFormProps> {

  pollCount: number = 0;

  form: RefObject<ValidatorForm>;

  saveData: () => void 


  constructor(props: DisplaySettingsFormProps) {
    super(props);
    // create a ref to store the textInput DOM element
    this.form = React.createRef();
    const { saveData } = this.props;
    this.saveData = saveData;
  }

  componentDidMount = () => {
    ValidatorForm.addValidationRule('isValidScreens', isValidScreens);
  }

  toUpperCase = (name: keyof DisplaySettings, valueSetter: (name: string, val: any)=>void ) => (event: React.ChangeEvent<HTMLInputElement>) => {
    let val = event.target.value as string
    valueSetter(name, val.toUpperCase())
  }

  render() {
    const { data, handleValueChange, handleDirectValueChange }  = this.props;

    return (
        <ValidatorForm onSubmit={this.saveData} ref={this.form}>
          <TextValidator
              validators={['required','isValidScreens']}
              errorMessages={['Display Screens is required','Invalid screen code']}
              name="screens"
              label="Display Screens"
              fullWidth
              variant="outlined"
              value={data.screens}
              onChange={this.toUpperCase('screens', handleDirectValueChange)}
              margin="normal"
              inputProps={{ style: { textTransform: "uppercase" } }}  
              helperText="Screens (letter is in brackets): (A)nalog time; (D)igital time; (I)ndoor temperature/humidity; (T)emperature chart; Covid-19 (S)pread risk; (O)utdoor weather; weather (F)orecast; (W)ind forecast; (M)oon phase rise/set; (C)onfiguration info"
            />
          <TextValidator
              validators={['required', 'isNumber', 'minNumber:1']}
              errorMessages={['Screen Rotate Interval is required', 'Must be a number at least 1', 'Must be a number at least 1']}
              name="screenRotateInterval"
              label="Screen Rotate Interval (in seconds)"
              fullWidth
              variant="outlined"
              value={data.screenRotateInterval}
              onChange={handleValueChange('screenRotateInterval')}
              margin="normal"
              helperText="How long a screen on display stays before changes"
            />
          
          <FormActions>
            <FormButton startIcon={<SaveIcon />} variant="contained" color="primary" type="submit">
              Save
            </FormButton>
          </FormActions>
        </ValidatorForm>
      )
  }

 
}

export default withStyles(styles)(DisplaySettingsForm);
