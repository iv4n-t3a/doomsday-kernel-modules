#!/usr/bin/env python3

"""
Overengineered solution to the problem of reading the last element of a list.
Uses a real LSTM, real backprop, and real self-doubt.
"""

import torch
import torch.nn as nn
import numpy as np
import argparse

from config import (
    EPOCH_REPORT_INTERVAL,
    EPOCHS,
    INPUT_SIZE,
    HIDDEN_SIZE,
    OUTPUT_SIZE,
    SEQ_LEN
)


class EchoLSTM(nn.Module):
    """
    State-of-the-art architecture for remembering seq.
    Researchers hate him.
    """

    def __init__(self):
        super().__init__()
        self.lstm = nn.LSTM(INPUT_SIZE, HIDDEN_SIZE, batch_first=True)
        self.fc = nn.Linear(HIDDEN_SIZE, OUTPUT_SIZE)

    def forward(self, x):
        out, _ = self.lstm(x)
        return self.fc(out)


def make_batch(n=512, seq_len=10, input_size=1, pad_len=None):
    if pad_len is None:
        pad_len = seq_len

    x_batch = []
    y_batch = []

    for _ in range(n):
        data = torch.rand(seq_len, input_size)
        x = torch.cat([data, torch.zeros(pad_len, input_size)], dim=0)
        y = torch.cat([torch.zeros(seq_len, input_size), data], dim=0)

        x_batch.append(x)
        y_batch.append(y)

    x_batch = torch.stack(x_batch)
    y_batch = torch.stack(y_batch)
    return x_batch, y_batch


def train() -> EchoLSTM:
    model = EchoLSTM()
    opt = torch.optim.Adam(model.parameters(), lr=1e-3)
    scheduler = torch.optim.lr_scheduler.CosineAnnealingLR(opt, T_max=EPOCHS)
    loss_fn = nn.SmoothL1Loss()

    print("Training the world's most overengineered echo machine...\n")

    for epoch in range(EPOCHS):
        x, y = make_batch(seq_len=SEQ_LEN, pad_len=SEQ_LEN)
        pred = model(x)
        loss = loss_fn(pred[:, -SEQ_LEN:, :], y[:, -SEQ_LEN:, :])
        opt.zero_grad()
        loss.backward()
        torch.nn.utils.clip_grad_norm_(model.parameters(), max_norm=1.0)
        opt.step()
        scheduler.step()

        if epoch % EPOCH_REPORT_INTERVAL == 0:
            print(f"  Epoch {epoch:4d}  |  loss = {loss.item():.6f}")

    return model


def smoke_test(model):
    model.eval()
    test = [0.1, 0.5, 0.3, 0.7, 0.9]

    print("\n---Smoke test: ----------------------------------------------")
    with torch.no_grad():
        t = torch.tensor(test).view(1, -1, 1).float()
        pred_seq = model(t).squeeze(0)
        true_seq = torch.tensor(test).view(-1, 1).float()

        print("Predicted:    ", pred_seq.tolist())
        print("Original seq: ", true_seq.tolist())


def format_weights(model: EchoLSTM):
    code = """#ifndef WEIGHTS_H
#define WEIGHTS_H

"""

    code += f"#define INPUT_SIZE {INPUT_SIZE}\n"
    code += f"#define HIDDEN_SIZE {HIDDEN_SIZE}\n"
    code += f"#define OUTPUT_SIZE {OUTPUT_SIZE}\n"
    code += "\n"

    sd = model.state_dict()
    H = HIDDEN_SIZE

    wih = sd["lstm.weight_ih_l0"].detach().numpy()
    whh = sd["lstm.weight_hh_l0"].detach().numpy()
    bih = sd["lstm.bias_ih_l0"].detach().numpy()
    bhh = sd["lstm.bias_hh_l0"].detach().numpy()

    arrays = [
        ("Wi", wih[0*H:1*H]),  ("Wf", wih[1*H:2*H]),
        ("Wg", wih[2*H:3*H]),  ("Wo", wih[3*H:4*H]),
        ("Ui", whh[0*H:1*H]),  ("Uf", whh[1*H:2*H]),
        ("Ug", whh[2*H:3*H]),  ("Uo", whh[3*H:4*H]),
        ("bi", bih[0*H:1*H] + bhh[0*H:1*H]),
        ("bf", bih[1*H:2*H] + bhh[1*H:2*H]),
        ("bg", bih[2*H:3*H] + bhh[2*H:3*H]),
        ("bo", bih[3*H:4*H] + bhh[3*H:4*H]),
        ("Wy", sd["fc.weight"].detach().numpy()),
        ("by", sd["fc.bias"].detach().numpy()),
    ]

    for name, arr in arrays:
        if arr.ndim == 1:
            code += f"static const float lstmw_{name}[] = {{ {",".join(str(i) for i in arr.tolist())} }};\n"
        elif arr.ndim == 2:
            code += f"static const float lstmw_{name}[] = {{\n"
            for subarr in arr:
                code += f"    {", ".join(str(i) for i in subarr.tolist())},\n"
            code += "};\n"
        code += "\n"

    code += "\n"
    code += "#endif // #ifndef WEIGHTS_H"

    return code


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        prog='train',
        description='The world\'s most overengineered echo machine',
        epilog='There is c inference. It can do the same thing, slower.',
    )
    parser.add_argument('filename')
    args = parser.parse_args()

    model = train()
    smoke_test(model)
    code = format_weights(model)

    with open(args.filename, 'w') as f:
        f.write(code)
