import style from '../ItkVtkViewer.module.css'

import screenshotIcon from '../../Icons/screenshot.svg'
import applyContrastSensitiveStyleToElement from '../applyContrastSensitiveStyleToElement'

function createScreenshotButton(context, mainUIRow) {
  const screenshotButton = document.createElement('div')
  screenshotButton.innerHTML = `<input id="${context.id}-screenshotButton" type="checkbox" checked class="${style.toggleInput}"><label itk-vtk-tooltip itk-vtk-tooltip-top-annotations itk-vtk-tooltip-content="Screenshot" class="${style.screenshotButton} ${style.toggleButton}" for="${context.id}-screenshotButton">${screenshotIcon}</label>`
  const screenshotButtonInput = screenshotButton.children[0]
  const screenshotLabel = screenshotButton.children[1]
  applyContrastSensitiveStyleToElement(
    context,
    'invertibleButton',
    screenshotLabel
  )

  screenshotButton.addEventListener('click', event => {
    event.preventDefault()
    event.stopPropagation()
    context.service.send('TAKE_SCREENSHOT')
    screenshotButton.checked = true
  })
  context.main.screenshotButton = screenshotButton
  mainUIRow.appendChild(screenshotButton)
}

export default createScreenshotButton
