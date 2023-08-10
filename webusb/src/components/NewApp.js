import { useState } from 'react'
import "./NewApp.css"

const NewPopup = () => {
    const [show, setShow] = useState(false);

    return (
        <div>
            
        </div>
    )
}

const NewApp = () => {
    return(
        <div className="new-app">
            <div className="u-flex u-column list-files-container">
                <div className="u-flex file-option-container">
                    <div className="file-name-container">
                        <div className="h3">
                                imx-image-core-imx8qxpc0mek-20230725120337.rootfs.wic
                        </div>
                    </div>
                    <div className="file-action-container">
                        <button class="button">Download</button>
                        <button class="button">USB Download</button>
                    </div>
                </div>
            </div>
        </div>
    )
}

export default NewApp