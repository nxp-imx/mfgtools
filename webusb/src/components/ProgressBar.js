import {useEffect, useState} from 'react';

const ProgressBar = (props) => {
    const [soFar, setSoFar] = useState();
    const [total, setTotal] = useState();

    const Parentdiv = {
        height: 20,
        width: '100%',
        backgroundColor: 'whitesmoke',
    }

    const Childdiv = {
        height: '100%',
        width: `${total? soFar/total*100: 0}%`,
        backgroundColor: 'green',
        textAlign: 'right'
    }

    useEffect(()=> {
        setTotal(props.total);
        setSoFar(props.soFar);
    }, [props])

    return(
        <div>
            <div
                className = "bar"
                style = {Parentdiv}
            >
                <div style = {Childdiv}></div>
            </div>
            {
                soFar&&total?
                (soFar===total? "Done":
                <span>{soFar} out of {total} bytes downloaded</span>):""
            }
        </div>
    )
}

export default ProgressBar